"""Main Class"""

import sys
import os
import aiomqtt
import asyncio
import json
from datetime import datetime
from decouple import config as env
from db.database import DB
from db.models import DataByTwo
from db.data_by_two import add_data_by_two

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

# ---------------------------------------------------------------------------------

class Target:
    id: int
    previous_position: tuple[float, float]
    current_position: tuple[float, float]
    previous_distance: float
    current_distance: float

    def __init__(self, id: int, previous_position: tuple[float, float], current_position: tuple[float, float], previous_distance: float, current_distance: float):
        self.id = id
        self.previous_position = previous_position
        self.current_position = current_position

        self.previous_distance = previous_distance
        self.current_distance = current_distance

database = DB()
monitoring_vector = [
    (float(env("D0_X")), float(env("D0_Y"))),
    (float(env("D1_X")), float(env("D1_Y")))
]  # Monitoring Line defined by two points
enter_exited_inverted = bool(int(env('ENTER_EXIT_INVERTED')))  # Flag to invert enter and exit detection
traversed_detection_count = 0  # Number of targets detected in the area
entered_detection_count = 0  # Number of targets entered the area
exited_detection_count = 0  # Number of targets exited the area

detected_indices = set()  # Indices of detected targets
targets: list[Target] = []

def point_to_line_distance(x, y, line):
    """Calculate signed distance of a point to a line."""
    (x1, y1), (x2, y2) = line
    numerator = (y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1
    denominator = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    return numerator / denominator if denominator != 0 else 0

def check_threshold(cur_pos, prev_pos, threshold: float=1.0) -> bool:
    """Check if the difference between the current and previous position is greater than a threshold."""
    return abs(cur_pos - prev_pos) > threshold

async def process_payload(message):
    """Process incoming MQTT payload."""
    global traversed_detection_count, entered_detection_count, exited_detection_count, enter_exited_inverted
    global targets

    try:
        payload = json.loads(message.payload.decode("utf-8"))
    except json.JSONDecodeError:
        print("Invalid JSON")
        return

    # Process each target
    for index in range(0, 5):
        targets[index].current_position = (float(payload[str(f't_{index}')]["x"]), float(payload[str(f't_{index}')]["y"]))

        if targets[index].current_position == (0, 0):
            # Mark target as 0 in the previous distances
            targets[index].previous_position = (0, 0)
            targets[index].previous_distance = 0
            continue

        targets[index].current_distance = point_to_line_distance(targets[index].current_position[0], targets[index].current_position[1], monitoring_vector)

        # Check if target was previously detected
        if check_threshold(targets[index].current_distance, targets[index].previous_distance, 1.2):
            print(f"{datetime.now()} Target <{index}> made an invalid movement")
            targets[index].previous_position = targets[index].current_position
            targets[index].previous_distance = targets[index].current_distance
            continue

        change = targets[index].current_distance * targets[index].previous_distance
        # Detect crossing (sign change in distance)
        if change == 0:
            continue
        if change < 0:
            print(f"{datetime.now()} Target <{index}> traversed", end=" -> ")
            traversed_detection_count += 1
            if targets[index].current_distance > 0:
                if enter_exited_inverted:
                    print(f"exited")
                    exited_detection_count += 1
                else:
                    print(f"entered")
                    entered_detection_count += 1
            else:
                if enter_exited_inverted:
                    print(f"entered")
                    entered_detection_count += 1
                else:
                    print(f"exited")
                    exited_detection_count += 1
        
        targets[index].previous_position = targets[index].current_position
        targets[index].previous_distance = targets[index].current_distance


async def send_buffer_to_db():
    """Send accumulated data to the database periodically."""
    while True:
        await asyncio.sleep(5)
        global traversed_detection_count, entered_detection_count, exited_detection_count
        if traversed_detection_count == 0:
            with database.get_db() as db:
                continue
        with database.get_db() as db:
            data = DataByTwo(sensor_id="D86C", traversed=traversed_detection_count, entered=entered_detection_count, exited=exited_detection_count)
            add_data_by_two(db, data)
            traversed_detection_count = 0
            entered_detection_count = 0
            exited_detection_count = 0

async def _main():
    """Main function."""
    global targets
    # Start the database
    database.create_all()

    asyncio.create_task(send_buffer_to_db())

    for target_id in range(0, 5):
        targets.append(
            Target(
                id=target_id,
                previous_position=(0, 0),
                current_position=(0, 0),
                previous_distance=0,
                current_distance=0
            )
        )

    while True:
        try:
            async with aiomqtt.Client(env("CLOUD-MQTT"), 1883) as client:
                await client.subscribe("SETE/sensors/sete003/D86C/raw")
                while True:
                    async for message in client.messages:
                        await process_payload(message)
        except Exception as e:
            print(e)

if __name__ == "__main__":
    print("Data Detection 2.0")
    print(f"Monitoring Line: {monitoring_vector}")
    print(f"Enter/Exit Inverted: {enter_exited_inverted}")
    asyncio.run(_main())
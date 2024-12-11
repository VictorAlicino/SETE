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

database = DB()
monitoring_vector = [
    (float(env("D0_X")), float(env("D0_Y"))),
    (float(env("D1_X")), float(env("D1_Y")))
]  # Monitoring Line defined by two points
enter_exited_inverted = bool(int(env('ENTER_EXIT_INVERTED')))  # Flag to invert enter and exit detection
detected_indices = set()  # Indices of detected targets
traversed_detection_count = 0  # Number of targets detected in the area
entered_detection_count = 0  # Number of targets entered the area
exited_detection_count = 0  # Number of targets exited the area
previous_distances = {}  # Store previous distances for each target

def point_to_line_distance(x, y, line):
    """Calculate signed distance of a point to a line."""
    (x1, y1), (x2, y2) = line
    numerator = (y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1
    denominator = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    return numerator / denominator if denominator != 0 else 0

async def process_payload(message):
    """Process incoming MQTT payload."""
    global traversed_detection_count, previous_distances, entered_detection_count, exited_detection_count, enter_exited_inverted

    try:
        payload = json.loads(message.payload.decode("utf-8"))
    except json.JSONDecodeError:
        print("Invalid JSON")
        return
    # Process each target
    for index, target in payload.items():
        if (target['x'] == 0 and target['y'] == 0):
            continue
        t_x, t_y = float(target["x"]), float(target["y"])
        current_distance = point_to_line_distance(t_x, t_y, monitoring_vector)
        # Check if target was previously detected
        if index in previous_distances:
            change = current_distance * previous_distances[index]
            # Detect crossing (sign change in distance)
            if change == 0:
                continue
            if change < 0:
                print(f"{datetime.now()} Target <{index}> traversed", end=" -> ")
                if current_distance > 0:
                    if enter_exited_inverted:
                        print(f"{datetime.now()} Target <{index}> exited", end="")
                        exited_detection_count += 1
                    else:
                        print(f"{datetime.now()} Target <{index}> entered", end="")
                        entered_detection_count += 1
                else:
                    if enter_exited_inverted:
                        print(f"{datetime.now()} Target <{index}> entered", end="")
                        entered_detection_count += 1
                    else:
                        print(f"{datetime.now()} Target <{index}> exited", end="")
                        exited_detection_count += 1
                traversed_detection_count += 1
                print()
        # Update previous distance
        previous_distances[index] = current_distance

async def send_buffer_to_db():
    """Send accumulated data to the database periodically."""
    while True:
        await asyncio.sleep(5)
        global traversed_detection_count, entered_detection_count, exited_detection_count
        if traversed_detection_count == 0:
            with database.get_db() as db:
                continue
        with database.get_db() as db:
            data = DataByTwo()
            data.sensor_id = "D80C"
            data.traversed = traversed_detection_count
            data.entered = entered_detection_count
            data.exited = exited_detection_count
            add_data_by_two(db, data)
            traversed_detection_count = 0
            entered_detection_count = 0
            exited_detection_count = 0

async def _main():
    """Main function."""
    # Start the database
    database.create_all()

    asyncio.create_task(send_buffer_to_db())

    while True:
        try:
            async with aiomqtt.Client(env("CLOUD-MQTT"), 1883) as client:
                await client.subscribe("SETE/sensors/sete003/D80C/raw")
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
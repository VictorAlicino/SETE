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
detected_indices = set()  # Indices of detected targets
area_detection_count = 0  # Number of targets detected in the area
previous_distances = {}  # Store previous distances for each target


def point_to_line_distance(x, y, line):
    """Calculate signed distance of a point to a line."""
    (x1, y1), (x2, y2) = line
    numerator = (y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1
    denominator = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    return numerator / denominator if denominator != 0 else 0


async def process_payload(message):
    """Process incoming MQTT payload."""
    global area_detection_count, previous_distances

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
            previous_distance = previous_distances[index]
            change = current_distance * previous_distance
            # Detect crossing (sign change in distance)
            if change == 0:
                continue
            if change < 0:
                if current_distance > 0:
                    print(f"{datetime.now()} Target <{index}> exited")
                else:
                    print(f"{datetime.now()} Target <{index}> entered")
                area_detection_count += 1
        # Update previous distance
        previous_distances[index] = current_distance

async def send_buffer_to_db():
    """Send accumulated data to the database periodically."""
    while True:
        await asyncio.sleep(5)
        global area_detection_count
        if area_detection_count == 0:
            # print(f"{datetime.now()} No targets detected in the area")
            continue
        with database.get_db() as db:
            data = DataByTwo()
            data.sensor_id = "D80C"
            data.traversed = area_detection_count
            #add_data_by_two(db, data)
            area_detection_count = 0


async def _main():
    """Main function."""
    # Start the database
    database.create_all()

    asyncio.create_task(send_buffer_to_db())

    while True:
        try:
            async with aiomqtt.Client(env("CLOUD-MQTT"), 1883) as client:
                await client.subscribe("SETE/sensors/test/sete003/D87C/raw")
                while True:
                    async for message in client.messages:
                        await process_payload(message)
        except Exception as e:
            print(e)


if __name__ == "__main__":
    print(f"Monitoring Line: {monitoring_vector}")
    asyncio.run(_main())
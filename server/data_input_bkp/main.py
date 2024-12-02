"""Main Class"""

import sys
import os
import aiomqtt
import asyncio
import json
import uuid as UUID
from datetime import datetime

import matplotlib.path as mpath
from db.database import DB
from db.models import DataByTwo
from db.data_by_two import add_data_by_two

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

# ---------------------------------------------------------------------------------

database = DB()
monitoring_area = [(-4, 2), (-4, 0.5), (4, 0.5), (4, 2)] # Monitoring Area
polygon_path = mpath.Path(monitoring_area)               # Monitor Area Path
detected_indices = set()                                 # Indices of detected targets
area_detection_count = 0                                 # Number of targets detected in the area

def is_point_in_area(x, y):
    x = float(x)
    y = float(y)
    return polygon_path.contains_point((x, y))

async def process_payload(message):
    # Check if the payload is a valid JSON
    try:
        payload = json.loads(message.payload.decode("utf-8"))
    except json.JSONDecodeError:
        print("Invalid JSON")
        return
    
    for index, target in enumerate(payload["targets"]):
        if is_point_in_area(target['x'], target['y']) and index not in detected_indices:
            detected_indices.add(index)
            area_detection_count += 1
        elif not is_point_in_area(target['x'], target['y']) and index in detected_indices:
            detected_indices.remove(index)

async def send_buffer_to_db():
    global area_detection_count
    while True:
        await asyncio.sleep(5)
        with database.get_db() as db:
            data = DataByTwo()
            data.sensor_id = "D80C"
            data.traversed = area_detection_count
            add_data_by_two(db, data)
            area_detection_count = 0

async def _main():
    # Start the database
    database.create_all()

    asyncio.create_task(send_buffer_to_db())

    while True:
        try:
            async with aiomqtt.Client("144.22.195.55", 1883) as client:
                await client.subscribe("SETE/sensors/sete003/D80C/raw")
                while True:
                    async for message in client.messages:
                        await process_payload(message)
        except Exception as e:
            print(e)

if __name__ == "__main__":
    asyncio.run(_main())
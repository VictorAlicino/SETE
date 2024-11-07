"""Main class"""
from db.database import DB
from db.models import SensorData, SensorInternalData, SensorLog
from db.sensor_data import add_sensor_data
from db.sensor_internal_data import add_sensor_internal_data
from db.sensor_log import add_sensor_log
import json
import logging
import aiomqtt
import sys
import asyncio
from datetime import datetime
import os

database = DB()
log = logging.getLogger(__name__)

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

async def store_sensor_log(payload, topic):
    # Define o caminho do diretório e do arquivo
    #db = next(database.get_db())
    #sensor_log = SensorLog(
    #    sensor_id=topic[3],
    #    timestamp=datetime.now(),
    #    log=payload
    #)
    # add_sensor_log(db, sensor_log)
    directory = f"sensor_log/{topic[2]}"
    filepath = f"{directory}/{topic[3]}.log"
    
    # Cria o diretório se não existir
    os.makedirs(directory, exist_ok=True)  # Cria o diretório, se necessário
    
    # Abre o arquivo no modo append
    with open(filepath, "a") as f:
        log_entry = f"{datetime.now()} {payload} \n"
        print(f"{topic[3]}: {log_entry[:-1]}")  # Imprime no console
        f.write(log_entry)  # Escreve no arquivo

async def sensor003_payload(payload, topic: list[str]):
    #print(f"{topic[3]} | {topic[4]}: {payload.payload.decode()}")
    match topic[4]:
        case "data":
            data = json.loads(payload.payload.decode())
            sensor_data = SensorData(
                sensor_id=topic[3],
                timestamp=datetime.now(),
                entered=data["entered"],
                exited=data["exited"],
                gave_up=data["gave_up"]
            )
            with database.get_db() as db:
                add_sensor_data(db, sensor_data)
        case "info":
            data = json.loads(payload.payload.decode())
            sensor_internal_data = SensorInternalData(
                sensor_id=topic[3],
                timestamp=datetime.now(),
                internal_temperature=data["internal_temperature"],
                free_memory=data["free_memory"],
                rssi=data["rssi"],
                uptime=data["uptime"],
                last_boot_reason=data["last_boot_reason"]
            )
            with database.get_db() as db:
                add_sensor_internal_data(db, sensor_internal_data)
        case "log":
            await store_sensor_log(payload.payload.decode(), topic)
        case _:
            print(f"Unknown Topic: {topic[4]}")

async def process_payload(payload):
    topic = str(payload.topic).split("/")
    match topic[2]:
        case "sete003":
            await sensor003_payload(payload, topic)
        case _:
            print(f"Unknown Sensor: {topic[2]}")

async def _main():
    # Start the database
    database.create_all()

    while True:
        try:
            async with aiomqtt.Client("144.22.195.55", 1883) as client:
                await client.subscribe("SETE/sensors/#")
                while True:
                    async for message in client.messages:
                        await process_payload(message)
        except Exception as e:
            print(e)

if __name__ == "__main__":
    asyncio.run(_main())
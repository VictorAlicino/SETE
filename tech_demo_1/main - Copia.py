import aiomqtt
import asyncio
import json
from datetime import datetime

# Use only if system is Windows
# We're using MacOs, so we don't need this line
# ----------------------
from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
set_event_loop_policy(WindowsSelectorEventLoopPolicy())

async def decode_payload(payload: json) -> None:
    now = datetime.now()
    for key, value in payload.items():
        targets: list = payload[key]['ld2461']['detections']
        pir = payload[key]['pir']['detection']

        if pir == 1:
            pir = "Movimento Detectado    "
        else:
            pir = "Movimento Não Detectado"

    print(f"{now.strftime("[%d/%m/%Y %H:%M:%S:%f]")} Sonare[{key}] -> PIR: {pir} | Pessoas Encontradas: {targets}")


async def main():
    async with aiomqtt.Client("144.22.195.55") as client:
        await client.subscribe("SETE/tech_demo")
        while True:
            async for message in client.messages:
                #print(f"Received message: {message.payload.decode()}")
                payload = json.loads(
                    message.payload.decode()
                )

                await decode_payload(payload)


if __name__ == "__main__":
    asyncio.run(main())

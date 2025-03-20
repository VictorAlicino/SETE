import aiomqtt
import os
import sys
import asyncio
from datetime import datetime

payload = []

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

mqtt_topic = "SETE/sensors/test_env/D850/data"

async def main():
    async with aiomqtt.Client("ssh.alicino.com.br", 1883) as client:
        # Simulates the JSON messages being sent at the seconds and microssends they were sent
        # f_message = first message | l_message = last message
        start_time = datetime.now()
        l_message_time = datetime.strptime(payload[-1][:26], "%Y-%m-%d %H:%M:%S.%f")
        f_message_time = datetime.strptime(payload[0][:26], "%Y-%m-%d %H:%M:%S.%f")
        time_remaining = (start_time - datetime.now()) - (f_message_time - l_message_time)
        print(f"this will take {time_remaining}")
        print(f"Simulation time: {start_time.strftime("%H:%M:%S")} - {(start_time + time_remaining).strftime("%H:%M:%S")}")
        print(f"Time remaining: {time_remaining}", end="")
        await client.publish(mqtt_topic, payload[0][27:])
        for message in payload[1:]:
            # Get the seconds from the message
            message_time = datetime.strptime(message[:26], "%Y-%m-%d %H:%M:%S.%f")
            ready: bool = False
            while not ready:
                # print(f"Seconds: {seconds}, Microseconds: {microseconds}")
                time_remaining = (start_time - datetime.now()) - (f_message_time - l_message_time)
                if (start_time - datetime.now()) < (f_message_time - message_time):
                    ready = True
                    break
                await asyncio.sleep(0.01)
                print(f"\r{message_time} | Time remaining: {time_remaining}", end="")
            await client.publish(mqtt_topic, message[27:])
    print("\nSimulation completed!")

if __name__ == "__main__":
    from tkinter import filedialog
    print("Select the file with the radar samples")
    file_path = filedialog.askopenfilename()
    with open(file_path, "r") as file:
        payload = file.readlines()
    print(f"Found {len(payload)} radar samples")
    print("Sending samples,", end=" ")
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nSimulation interrupted!")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        print("Closing the program...")
        sys.exit(0)

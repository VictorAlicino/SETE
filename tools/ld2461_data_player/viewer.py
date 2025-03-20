import sys
import aiomqtt
import json
import asyncio
import threading
import tkinter as tk
from tkinter import ttk, filedialog
import matplotlib.pyplot as plt
from datetime import datetime
from decouple import config as env
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.patches import Polygon
import os
from db.database import DB
from db.sensor_internal_data import get_distinct_sensor

detection_area = []
detection_line = []

sample_data_stream = []

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

# Função para processar os dados do payload e atualizar o gráfico
async def process_payload(message):
    try:
        payload = json.loads(message)
    except Exception as e:
        print(f"\n{message}", end="")
        raise e
    
    # Extrair os pontos de coordenadas
    t0 = payload["t_0"]['x'], payload["t_0"]['y']
    t1 = payload["t_1"]['x'], payload["t_1"]['y']
    t2 = payload["t_2"]['x'], payload["t_2"]['y']
    t3 = payload["t_3"]['x'], payload["t_3"]['y']
    t4 = payload["t_4"]['x'], payload["t_4"]['y']

    # Atualizar o gráfico com esses pontos
    update_graph([t0, t1, t2, t3, t4])

# Função para atualizar o gráfico com novos pontos
def update_graph(detections):
    global ax

    # Limpar o gráfico atual
    ax.clear()

    # Definir o limite do gráfico
    ax.set_xlim([-10, 10])
    ax.set_ylim([0, 8])

    ax.grid(True, which='both', linestyle='--', linewidth=0.5, color='gray')

    # Adicionar polígono com os pontos de detection_area
    if len(detection_area) == 4:
        polygon = Polygon(detection_area, closed=True, edgecolor='red', facecolor='red', alpha=0.1)
        ax.add_patch(polygon)

    # Adicionar linha de detecção
    if len(detection_line) == 2:
        ax.plot([detection_line[0][0], detection_line[1][0]], [detection_line[1][1], detection_line[0][1]], 'r-', linewidth=2)

    # Adicionar os pontos ao gráfico
    for detection in detections:
        ax.plot(detection[0], detection[1], 'bo', markersize=8)

    for index, detection in enumerate(detections):
        x, y = detection
        if x == 0 and y == 0:
            continue
        ax.plot(x, y, 'bo', markersize=2)
        ax.annotate(f"T{index} ({x}, {y})", (x, y), textcoords="offset points", xytext=(0, 10), ha='center')

    canvas.draw()

async def loop():
    await get_detection_area()
    print(f"Detection Area: {detection_area}\nDetection Line: {detection_line}")
    try:
        start_time = datetime.now()
        l_message_time = datetime.strptime(sample_data_stream[-1][:26], "%Y-%m-%d %H:%M:%S.%f")
        f_message_time = datetime.strptime(sample_data_stream[0][:26], "%Y-%m-%d %H:%M:%S.%f")
        time_remaining = (start_time - datetime.now()) - (f_message_time - l_message_time)
        print(f"Starting reconstitution with {len(sample_data_stream)} samples")
        print(f"this will take {time_remaining}")
        print(f"Reconstitution time: {f_message_time} - {l_message_time}")
        print(f"{l_message_time} | Time remaining: {time_remaining}", end="")
        await process_payload(sample_data_stream[0][27:])
        for message in sample_data_stream[1:]:
            # Get the seconds from the message
            message_time = datetime.strptime(message[:26], "%Y-%m-%d %H:%M:%S.%f")
            try:
                next_message_time = datetime.strptime(sample_data_stream[sample_data_stream.index(message) + 1][:26], "%Y-%m-%d %H:%M:%S.%f")
            except IndexError:
                next_message_time = datetime.strptime(sample_data_stream[-1][:26], "%Y-%m-%d %H:%M:%S.%f")
            ready: bool = False
            while not ready:
                # print(f"Seconds: {seconds}, Microseconds: {microseconds}")
                time_remaining = (start_time - datetime.now()) - (f_message_time - l_message_time)
                if (start_time - datetime.now()) < (f_message_time - next_message_time):
                    ready = True
                    break
                await asyncio.sleep(0.01)
                print(f"\r{message_time} | Time remaining: {time_remaining}", end="")
            await process_payload(message[27:])
        print("\rReconstitution completed!                                          ")
    except Exception as e:
        print(f"\nSample processing error: {e}")
        raise e

def start_gui(): 
    global root, canvas, ax

    root = tk.Tk()
    root.title("Radar Data Viewer")

    graph_frame = tk.Frame(root)
    graph_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

    # Criar o gráfico
    fig = plt.Figure(figsize=(5, 4), dpi=100)
    ax = fig.add_subplot(111)

    # Criar o canvas para exibir o gráfico no tkinter
    canvas = FigureCanvasTkAgg(fig, master=graph_frame)
    canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    # Adicionar o botão de fechar
    button_quit = tk.Button(root, text="Sair", command=root.quit)
    button_quit.pack(side=tk.BOTTOM, pady=10)

    # Atualizar a interface gráfica a cada 100 ms
    root.after(100, update_gui)

    # Iniciar a interface gráfica
    root.mainloop()

# Função para atualizar a interface gráfica (ticking do loop do tkinter)
def update_gui():
    # Apenas chama o update novamente após 100ms
    root.after(100, update_gui)

async def get_detection_area():
    detection_area.append((float(env('D0_X')), float(env('D0_Y'))))
    detection_area.append((float(env('D1_X')), float(env('D1_Y'))))
    detection_area.append((float(env('D2_X')), float(env('D2_Y'))))
    detection_area.append((float(env('D3_X')), float(env('D3_Y'))))
    detection_line.append((float(env('S0_X')), float(env('S0_Y'))))
    detection_line.append((float(env('S1_X')), float(env('S1_Y'))))
    return

if __name__ == "__main__":
    data_stream = filedialog.askopenfilename(title="Select the radar samples file", filetypes=[("JSON Lines", "*.jsonl")])
    try:
        with open(data_stream, "r") as file:
            sample_data_stream = file.readlines()
    except Exception as e:
        print(f"Failed to opened file: {e}")
        sys.exit(1)

    try:
        thread1 = threading.Thread(target=lambda: asyncio.run(start_gui()))

        thread1.start()
        import time
        time.sleep(1)
        asyncio.run(loop())
        # Iniciar a interface gráfica
    except KeyboardInterrupt:
        print("\rReconstitution interrupted!")
    except Exception as e:
        print(f"An excepting was raised and the program will be closed \n[ {e} ]")
        thread1.join()
    finally:
        print("Closing the program...")
        sys.exit(0)

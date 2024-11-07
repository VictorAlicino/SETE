import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.patches as patches
import matplotlib.path as mpath
import os
import sys
import json
import aiomqtt
import asyncio
from datetime import datetime
from queue import Queue
import threading

# Change to the "Selector" event loop if platform is Windows
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

# Variáveis globais
current_data = {
    "selected_sensor": "",
    "radar": {"detections": []},
    "pir": {"detection": 0},
    "raw": "",
    "sensor_list": []
}

plt_styles = [
    'Solarize_Light2',
    '_classic_test_patch',
    '_mpl-gallery',
    '_mpl-gallery-nogrid',
    'bmh', 
    'classic',
    'dark_background',
    'fast',
    'fivethirtyeight',
    'ggplot',
    'grayscale',
    'seaborn-v0_8',
    'seaborn-v0_8-bright',
    'seaborn-v0_8-colorblind',
    'seaborn-v0_8-dark',
    'seaborn-v0_8-dark-palette',
    'seaborn-v0_8-darkgrid',
    'seaborn-v0_8-deep',
    'seaborn-v0_8-muted',
    'seaborn-v0_8-notebook',
    'seaborn-v0_8-paper',
    'seaborn-v0_8-pastel',
    'seaborn-v0_8-poster',
    'seaborn-v0_8-talk',
    'seaborn-v0_8-ticks',
    'seaborn-v0_8-white',
    'seaborn-v0_8-whitegrid',
    'tableau-colorblind10'
    ]

area_detection_count = 0
detected_indices = set()

monitoring_area = [(-4, 2), (-4, 0.5), (4, 0.5), (4, 2)]
polygon_path = mpath.Path(monitoring_area)

# Fila para comunicação entre threads
data_queue = Queue()

# Função para verificar se um ponto está dentro da área de monitoramento
def is_point_in_area(x, y):
    return polygon_path.contains_point((x, y))

# Função para atualizar o gráfico e a interface
def update_interface():
    global area_detection_count

    fig.clear()
    ax = fig.add_subplot(111)
    plt.style.use(plt_styles[1])

    ax.set_xlim([-5, 5])
    ax.set_ylim([0, 8])
    ax.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)

    polygon = patches.Polygon(monitoring_area, closed=True, fill=True, color='red', alpha=0.2)
    ax.text(0, 0.75, "Área de Monitoramento", fontsize=5, fontweight='regular', ha='center')
    ax.add_patch(polygon)

    ax.set_title("Radar HLK-LD2461", fontsize=10, fontweight='bold')

    for index, detection in enumerate(current_data["radar"]["detections"]):
        x = detection["x"]
        y = detection["y"]
        ax.plot(x, y, 'bo', markersize=8)
        ax.annotate(f"({x}, {y})", (x, y), textcoords="offset points", xytext=(0, 10), ha='center', fontsize=9)

        if is_point_in_area(x, y) and index not in detected_indices:
            detected_indices.add(index)
            area_detection_count += 1
        elif not is_point_in_area(x, y) and index in detected_indices:
            detected_indices.remove(index)

    pir_state = current_data["pir"]["detection"]
    pir_color = 'red' if pir_state == 1 else 'white'
    pir_circle.config(bg=pir_color)

    raw_data_text.config(state=tk.NORMAL)
    raw_data_text.delete(1.0, tk.END)
    raw_data = current_data["raw"]
    raw_data_text.insert(tk.END, raw_data)
    raw_data_text.config(state=tk.DISABLED)

    radar_status.config(text=f"Limite de detecção: {len(current_data['radar']['detections'])} de 5")
    area_status.config(text=f"Pessoas detectadas: {area_detection_count}")
    sensor_selector["values"] = current_data["sensor_list"]
    sensor_name.config(text=f"Sonare {current_data['selected_sensor']}" if current_data["selected_sensor"] else "Não Selecionado")
    
    canvas.draw()

# Função para processar dados recebidos na fila
def process_data():
    while True:
        if not data_queue.empty():
            data = data_queue.get()
            update_interface()

# Função para atualizar os dados do radar
def update_radar_data(detections):
    current_data["radar"]["detections"] = detections

# Função para atualizar os dados do PIR
def update_pir_data(detection):
    current_data["pir"]["detection"] = detection

# Função para atualizar os dados brutos
def update_raw_data(raw):
    current_data["raw"] = raw

# Função para atualizar os valores do menu dropdown
def update_sensor_selector(sensor_list):
    current_data["sensor_list"] = sensor_list
    sensor_selector["values"] = current_data["sensor_list"]  # Atualiza as opções do dropdown

# Interface gráfica
root = tk.Tk()
root.title("Sonare Techdemo")
root.geometry("")
root.configure(bg="#1c2e34")

# Título "Sonare" e "techdemo"
title_frame = tk.Frame(root, bg="#1c2e34")
title_frame.pack(side=tk.TOP, anchor=tk.NW, padx=10, pady=10)
title_label = tk.Label(title_frame, text="Sonare", font=("Helvetica", 32, "bold"), fg="white", bg="#1c2e34")
title_label.pack(side=tk.TOP)
subtitle_label = tk.Label(title_frame, text="techdemo", font=("Helvetica", 12), fg="white", bg="#1c2e34")
subtitle_label.pack(side=tk.RIGHT)

# Criar um novo frame para o gráfico, sensor selector e os status
graph_frame = tk.Frame(root, bg="#1c2e34")
graph_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=10)

# Menu Dropdown para selecionar o sensor, ajustado para ficar no mesmo nível do título
sensor_selector = ttk.Combobox(root, values=[])
sensor_selector.set("Selecione um sensor")
sensor_selector.place(x=990, y=15)  # Ajuste a posição de acordo com a necessidade

# Criar um novo frame para o gráfico e os status
graph_frame = tk.Frame(root, bg="#1c2e34")
graph_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=10)

# Criar a área do gráfico
fig = plt.Figure(figsize=(4, 3), dpi=150)

canvas = FigureCanvasTkAgg(fig, master=graph_frame)
canvas.get_tk_widget().pack(side=tk.LEFT, padx=10, pady=10)

# Status Frame (Radar e PIR display) ajustado para alinhar à esquerda do gráfico
status_frame = tk.Frame(graph_frame, bg="#1c2e34")
status_frame.pack(side=tk.LEFT, padx=15, pady=15)  # Usar LEFT para ficar ao lado do gráfico

# Sensor Selected Name
sensor_label = tk.Label(status_frame, text="Dados do sensor:", font=("Helvetica", 12), fg="white", bg="#1c2e34")
sensor_label.grid(row=0, column=0, sticky="w")
sensor_name = tk.Label(status_frame, text="Não Selecionado", font=("Helvetica", 22, "bold"), fg="white", bg="#1c2e34")
sensor_name.grid(row=1, column=0, sticky="w")

# Separador
separator = ttk.Separator(status_frame, orient="horizontal")
separator.grid(row=2, column=0, columnspan=2, sticky="ew", pady=10)

# Radar Status
radar_label = tk.Label(status_frame, text="Radar", font=("Helvetica", 18, "bold"), fg="white", bg="#1c2e34")
radar_label.grid(row=4, column=0, sticky="w")
radar_status = tk.Label(status_frame, text="Limite de detecção: 0 de 5", font=("Helvetica", 12), fg="white", bg="#1c2e34")
radar_status.grid(row=5, column=0, sticky="w")

# Infravermelho Status
pir_label = tk.Label(status_frame, text="Infravermelho", font=("Helvetica", 18, "bold"), fg="white", bg="#1c2e34")
pir_label.grid(row=6, column=0, sticky="w")
pir_circle = tk.Label(status_frame, width=5, height=2, bg="white", relief="solid")
pir_circle.grid(row=6, column=1, padx=5, sticky="w")

# Separador
separator = ttk.Separator(status_frame, orient="horizontal")
separator.grid(row=7, column=0, columnspan=2, sticky="ew", pady=15)

# Adicionar um label para mostrar o contador da área
area_status = tk.Label(status_frame, text="Pessoas detectadas: 0", font=("Helvetica", 30, "bold"), fg="white", bg="#1c5e34")
area_status.grid(row=8, column=0, sticky="w")

# Separador
separator = ttk.Separator(status_frame, orient="horizontal")
separator.grid(row=9, column=0, columnspan=2, sticky="ew", pady=10)

# RAW Data Frame
raw_data_frame = tk.Frame(root, bg="#1c2e34")
raw_data_frame.pack(side=tk.LEFT, padx=10, pady=10)

raw_data_label = tk.Label(raw_data_frame, text="Dados brutos:", font=("Helvetica", 12, "bold"), fg="white", bg="#1c2e34")
raw_data_label.pack(anchor=tk.NW)

# Quadro preto para mostrar dados brutos
raw_data_text = tk.Text(raw_data_frame, width=130, height=3, bg="black", fg="green", font=("Consolas", 12))
raw_data_text.pack(padx=5, pady=5)
raw_data_text.config(state=tk.DISABLED)

# Função para atualizar o sensor selecionado
def update_selected_sensor():
    current_data["selected_sensor"] = sensor_selector.get()
    sensor_name.config(text=f"Sonare {current_data['selected_sensor']}" if current_data["selected_sensor"] else "Não Selecionado")
    update_interface()  # Chama update_interface para refletir a seleção

sensor_selector.bind("<<ComboboxSelected>>", lambda event: update_selected_sensor())

# Função para decodificar a carga útil
async def decode_payload(payload: json) -> None:
    now = datetime.now()
    for key, value in payload.items():
        targets = payload['ld2461']['detections']
        pir = payload['pir']['detection']
        update_sensor_selector(["D87C"])

        if sensor_selector.get() == "D87C":
            update_raw_data(f"{now.strftime('[%d/%m/%Y %H:%M:%S:%f]')} {payload}")
            update_pir_data(pir)
            update_radar_data(targets)
        
        # Adicionar dados à fila
        data_queue.put(payload)

# Função para conectar ao MQTT
async def mqtt_client():
    async with aiomqtt.Client("144.22.195.55") as client:
        await client.subscribe("SETE/sensors/sete003/D87C/data")
        while True:
            async for message in client.messages:
                payload = json.loads(message.payload.decode())
                await decode_payload(payload)
                await asyncio.sleep(0.1)

# Função para integrar o asyncio com Tkinter
def run_mqtt():
    asyncio.run(mqtt_client())

# Iniciar o loop MQTT em um thread separado
mqtt_thread = threading.Thread(target=run_mqtt)
mqtt_thread.start()

# Iniciar a thread para processar dados
processing_thread = threading.Thread(target=process_data)
processing_thread.start()

# Loop principal da interface
root.mainloop()
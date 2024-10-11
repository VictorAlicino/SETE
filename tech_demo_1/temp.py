import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import json
import aiomqtt
import asyncio
from datetime import datetime

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

# Função para atualizar o gráfico e a interface
def update_interface():
    fig.clear()
    ax = fig.add_subplot(111)
    ax.set_xlim([-5, 5])
    ax.set_ylim([0, 8])
    
    # Adicionando uma grade
    ax.grid(True, which='both', linestyle='--', linewidth=0.5)

    # Definindo as divisões
    ax.set_xticks(range(-5, 6))
    ax.set_yticks(range(0, 9))

    # Atualizando o gráfico com os dados do radar
    for detection in current_data["radar"]["detections"]:
        x = detection["x"]
        y = detection["y"]
        ax.plot(x, y, 'bo')  # Plotando pontos em azul

    # Atualizando o status do infravermelho (PIR)
    pir_state = current_data["pir"]["detection"]
    pir_color = 'red' if pir_state == 1 else 'white'
    pir_circle.config(bg=pir_color)

    # Atualizando dados brutos
    raw_data_text.config(state=tk.NORMAL)
    raw_data_text.delete(1.0, tk.END)
    raw_data = current_data["raw"]
    raw_data_text.insert(tk.END, raw_data)
    raw_data_text.config(state=tk.DISABLED)

    # Atualizando o status do radar
    radar_status.config(text=f"Pessoas Detectadas: {len(current_data['radar']['detections'])} de 5")

    # Atualizando o status do PIR
    pir_status.config(text=f"Detecções: {current_data['pir']['detection']} de 1")

    # Atualizando o menu dropdown
    sensor_selector["values"] = current_data["sensor_list"]

    # Atualizando o nome do sensor selecionado
    if current_data["selected_sensor"] == "":
        sensor_name.config(text="Não Selecionado")
    else:
        sensor_name.config(text=f"SETE Sonare {current_data['selected_sensor']}")
    
    # Atualizar o gráfico
    canvas.draw()

# Função para atualizar os dados do radar
def update_radar_data(detections):
    current_data["radar"]["detections"] = detections
    update_interface()

# Função para atualizar os dados do PIR
def update_pir_data(detection):
    current_data["pir"]["detection"] = detection
    update_interface()

# Função para atualizar os dados brutos
def update_raw_data(raw):
    current_data["raw"] = raw
    update_interface()

# Função para atualizar os valores do menu dropdown
def update_sensor_selector(sensor_list):
    current_data["sensor_list"] = sensor_list
    update_interface()

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
fig = plt.Figure(figsize=(5, 4), dpi=150)
canvas = FigureCanvasTkAgg(fig, master=graph_frame)
canvas.get_tk_widget().pack(side=tk.LEFT, padx=10, pady=10)

# Status Frame (Radar e PIR display) ajustado para alinhar à esquerda do gráfico
status_frame = tk.Frame(graph_frame, bg="#1c2e34")
status_frame.pack(side=tk.LEFT, padx=15, pady=15)  # Usar LEFT para ficar ao lado do gráfico

# Sensor Selected Name
sensor_label = tk.Label(status_frame, text="Dados do sensor:", font=("Helvetica", 12), fg="white", bg="#1c2e34")
sensor_label.grid(row=0, column=0, sticky="w")
sensor_name = tk.Label(status_frame, text="Não Selecionado", font=("Helvetica", 20, "bold"), fg="white", bg="#1c2e34")
sensor_name.grid(row=1, column=0, sticky="w")

# Separador
separator = ttk.Separator(status_frame, orient="horizontal")
separator.grid(row=2, column=0, columnspan=2, sticky="ew", pady=10)

# Radar Status
radar_label = tk.Label(status_frame, text="Radar", font=("Helvetica", 18, "bold"), fg="white", bg="#1c2e34")
radar_label.grid(row=4, column=0, sticky="w")
radar_status = tk.Label(status_frame, text="Pessoas Detectadas: 0 de 5", font=("Helvetica", 12), fg="white", bg="#1c2e34")
radar_status.grid(row=5, column=0, sticky="w")

# Infravermelho Status
pir_label = tk.Label(status_frame, text="Infravermelho", font=("Helvetica", 18, "bold"), fg="white", bg="#1c2e34")
pir_label.grid(row=6, column=0, sticky="w")
pir_circle = tk.Label(status_frame, width=2, height=1, bg="white", relief="solid")
pir_circle.grid(row=6, column=1, padx=10, sticky="w")
pir_status = tk.Label(status_frame, text="Detecções: 0 de 1", font=("Helvetica", 12), fg="white", bg="#1c2e34")
pir_status.grid(row=7, column=0, sticky="w")

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
    update_interface()

async def decode_payload(payload: json) -> None:
    now = datetime.now()
    for key, value in payload.items():
        targets: list = payload[key]['ld2461']['detections']
        pir = payload[key]['pir']['detection']
        update_sensor_selector(list(payload.keys()))
        # Atualizar os dados do radar, PIR e dados brutos se o sensor selecionado for o mesmo que o sensor atual
        if sensor_selector.get() == key:
            update_selected_sensor()
            update_pir_data(pir)
            update_radar_data(targets)
            #update_raw_data(f"{now.strftime('[%d/%m/%Y %H:%M:%S:%f]')} Sonare[{key}] -> PIR: {pir} | Pessoas Encontradas: {targets}")
            update_raw_data(payload)

        #if pir == 1:
        #    pir = "Movimento Detectado    "
        #else:
        #    pir = "Movimento Não Detectado"

        print(f"{now.strftime("[%d/%m/%Y %H:%M:%S:%f]")} Sonare[{key}] -> PIR: {pir} | Pessoas Encontradas: {targets}")


# Função para conectar ao MQTT
async def mqtt_client():
    async with aiomqtt.Client("144.22.195.55") as client:
        await client.subscribe("SETE/#")
        while True:
            async for message in client.messages:
                payload = json.loads(message.payload.decode())
                await decode_payload(payload)
                await asyncio.sleep(0)  # Permite que o loop de eventos prossiga

# Função para integrar o asyncio com Tkinter
def run_mqtt():
    asyncio.run(mqtt_client())

# Iniciar o loop MQTT em um thread separado
import threading
mqtt_thread = threading.Thread(target=run_mqtt)
mqtt_thread.start()

# Loop principal da interface
root.mainloop()

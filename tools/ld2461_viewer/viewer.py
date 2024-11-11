import sys
import aiomqtt
import json
import asyncio
import threading
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import os
from db.database import DB
from db.sensor_internal_data import get_distinct_sensor

# MQTT Async Guard
if sys.platform.lower() == "win32" or os.name.lower() == "nt":
    from asyncio import set_event_loop_policy, WindowsSelectorEventLoopPolicy
    set_event_loop_policy(WindowsSelectorEventLoopPolicy())

# Conectar ao banco de dados e obter sensores disponíveis
database = DB()
selected_sensor = None

# Função para processar os dados do payload e atualizar o gráfico
async def process_payload(message):
    payload = json.loads(message.payload.decode())
    
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
    ax.set_xlim([-5, 5])
    ax.set_ylim([0, 8])

    ax.grid(True, which='both', linestyle='--', linewidth=0.5, color='gray')

    # Adicionar os pontos ao gráfico
    for detection in detections:
        ax.plot(detection[0], detection[1], 'bo', markersize=8)

    for index, detection in enumerate(detections):
        if x == 0 and y == 0:
            continue
        x, y = detection
        ax.plot(x, y, 'bo', markersize=2)
        ax.annotate(f"T{index} ({x}, {y})", (x, y), textcoords="offset points", xytext=(0, 10), ha='center')

    # Desenhar o gráfico novamente
    canvas.draw()

# Função para conectar ao MQTT e receber dados
async def mqtt_loop():
    try:
        async with aiomqtt.Client("144.22.195.55", 1883) as client:
            await client.subscribe(f"SETE/sensors/sete003/{selected_sensor}/raw")
            while True:
                async for message in client.messages:
                    await process_payload(message)
    except Exception as e:
        print(f"Erro no MQTT: {e}")

# Função para inicializar a interface gráfica
def start_gui():
    global root, canvas, ax

    # Criar a janela principal
    root = tk.Tk()
    root.title("Visualizador de Dados do Radar")

    # Criar um frame para o gráfico
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

# Função para rodar o loop MQTT em uma thread
def run_mqtt():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(mqtt_loop())

if __name__ == "__main__":
    # Conectar ao banco de dados e listar os sensores
    print("Selecione um sensor para visualizar o dado do radar em tempo real: ")
    database.attemp_connect()
    with database.get_db() as db:
        sensors = get_distinct_sensor(db)
    sensors = [sensor[0] for sensor in sensors]
    
    print("Sensores disponíveis no banco de dados:")
    for sensor in sensors:
        print(sensor)

    # Solicitar ao usuário a escolha do sensor
    selected_sensor = input("Digite o nome do sensor desejado: ")
    print("Conectando ao servidor MQTT...")

    # Criar e iniciar a thread para o MQTT
    mqtt_thread = threading.Thread(target=run_mqtt, daemon=True)
    mqtt_thread.start()

    # Iniciar a interface gráfica
    start_gui()

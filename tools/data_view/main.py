import pandas as pd
import matplotlib.pyplot as plt
import re
from tkinter import filedialog

# Configuração
csv_path = filedialog.askopenfilename(title="Selecione os dados brutos do radar")
log_path = filedialog.askopenfilename(title="Selecione o arquivo de logs")

# Definição da área de detecção
detection_area = {
    "D0": {"x": -2, "y": 3},
    "D1": {"x": -2, "y": 1.8},
    "D2": {"x": 2, "y": 1.8},
    "D3": {"x": 2, "y": 3}
}

# Carregar CSV
df = pd.read_csv(csv_path)
df["Time"] = pd.to_datetime(df["Time"])

df["Active_Targets"] = df[[f"T{i}_X" for i in range(5)]].ne(0).sum(axis=1)
df["Target_Diff"] = df["Active_Targets"].diff().fillna(0)

disappearance_events = (df["Target_Diff"] < 0).sum()

# Exibir distribuição de alvos detectados
print("Distribuição de alvos ativos por instante:")
print(df["Active_Targets"].value_counts().sort_index())
print(f"Número de desaparecimentos abruptos: {disappearance_events}")

# Criar subplots para cada target
fig, axes = plt.subplots(5, 1, figsize=(10, 15), sharex=True)
for i in range(5):
    axes[i].plot(df["T{}_X".format(i)], df["T{}_Y".format(i)], label=f"T{i} Trajetória", linestyle="-", marker="o", markersize=2)
    
    # Adicionar a área de detecção
    x_vals = [detection_area["D0"]["x"], detection_area["D1"]["x"], detection_area["D2"]["x"], detection_area["D3"]["x"], detection_area["D0"]["x"]]
    y_vals = [detection_area["D0"]["y"], detection_area["D1"]["y"], detection_area["D2"]["y"], detection_area["D3"]["y"], detection_area["D0"]["y"]]
    axes[i].fill(x_vals, y_vals, color='red', alpha=0.3, label="Área de Detecção")
    
    axes[i].set_ylabel("Y")
    axes[i].set_title(f"Target {i}")
    axes[i].legend()
    axes[i].grid()

axes[-1].set_xlabel("X")
plt.tight_layout()
plt.show()

# Processar logs
def process_logs(log_path):
    with open(log_path, "r", encoding="utf-8") as file:
        logs = file.readlines()
    
    detection_logs = [line for line in logs if "DETECTION" in line]
    pattern = re.compile(r"Target (\d+) .* entry point: \(([-\d.]+), ([-\d.]+)\) .* exit point: \(([-\d.]+), ([-\d.]+)\)")
    log_entries = []
    
    for log in detection_logs:
        match = pattern.search(log)
        if match:
            target_id, entry_x, entry_y, exit_x, exit_y = match.groups()
            log_entries.append({
                "Target": int(target_id),
                "Entry_X": float(entry_x),
                "Entry_Y": float(entry_y),
                "Exit_X": float(exit_x),
                "Exit_Y": float(exit_y),
            })
    return pd.DataFrame(log_entries)

log_df = process_logs(log_path)
print("Resumo das entradas e saídas nos logs:")
print(log_df.head())

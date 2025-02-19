import json
import csv
from tkinter import filedialog

def convert_jsonl_to_csv():
    # Seleciona o arquivo JSONL
    jsonl_file_path = filedialog.askopenfilename(title="Selecione o arquivo JSONL")
    if not jsonl_file_path:
        print("Nenhum arquivo selecionado.")
        return
    
    csv_data = []
    
    with open(jsonl_file_path, "r") as jsonl_file:
        print(f"Processando arquivo {jsonl_file_path}...")
        jsonl_lines = jsonl_file.readlines()
        total_lines = len(jsonl_lines)
        
        for index, line in enumerate(jsonl_lines, start=1):
            try:
                time, json_data = line.strip().split(";", 1)
                data = json.loads(json_data)
                
                row = [time]
                valid_data = []
                
                for i in range(5):
                    x, y = data[f't_{i}']['x'], data[f't_{i}']['y']
                    row.extend([x, y])
                    valid_data.append(not (x == 0.0 and y == 0.0))
                
                if any(valid_data):
                    csv_data.append(row)
            except (ValueError, KeyError, json.JSONDecodeError):
                print(f"Erro ao processar a linha {index}, ignorando...")
            
            percent = (index * 100) // total_lines
            print(f"\rProcessando: {percent}%", end="")
    
    print("\nProcessamento concluído.")
    
    # Seleciona onde salvar o arquivo CSV
    csv_file_path = filedialog.asksaveasfilename(defaultextension=".csv", filetypes=[("CSV files", "*.csv")])
    if not csv_file_path:
        print("Nenhum local de salvamento selecionado.")
        return
    
    # Salva os dados no arquivo CSV
    with open(csv_file_path, "w", newline="") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(["Time", "T0_X", "T0_Y", "T1_X", "T1_Y", "T2_X", "T2_Y", "T3_X", "T3_Y", "T4_X", "T4_Y"])
        writer.writerows(csv_data)
    
    print(f"CSV salvo com sucesso em: {csv_file_path}")

if __name__ == "__main__":
    convert_jsonl_to_csv()

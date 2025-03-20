import json
from tkinter import filedialog
import datetime

# Nome do arquivo de entrada e saída
input_file = filedialog.askopenfilename(title="Selecione o arquivo de entrada", filetypes=(("JSONL files", "*.jsonl"),))
output_file = filedialog.asksaveasfilename(title="Selecione o arquivo de saída", filetypes=(("JSONL files", "*.jsonl"),))

def filter_jsonl(input_file, output_file):
    last_timestamp = None
    
    with open(input_file, "r") as infile, open(output_file, "w") as outfile:
        for line in infile:
            try:
                timestamp_str, json_data = line.strip().split(";", 1)
                timestamp = datetime.datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S.%f")
                
                if last_timestamp is None or (timestamp - last_timestamp).total_seconds() >= 1:
                    outfile.write(line)
                    last_timestamp = timestamp
            except ValueError as e:
                print(f"Erro ao processar linha: {line.strip()} - {e}")

if __name__ == "__main__":
    filter_jsonl(input_file, output_file)
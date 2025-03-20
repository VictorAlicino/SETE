import os
import re
import sys

# Padrões para ignorar mensagens irrelevantes
IGNORE_PATTERNS = [
    r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+\s*$",  # Linhas vazias com timestamp
    r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ I \(\d+\) wifi",  # Linhas do WiFi
    r"^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+ \[ADDBA\]RX DELBA",  # Logs [ADDBA]
]

def should_ignore(line):
    """Verifica se a linha deve ser ignorada com base nos padrões."""
    return any(re.search(pattern, line) for pattern in IGNORE_PATTERNS)

def count_lines(file_path):
    """Conta o número total de linhas no arquivo (para exibir progresso)."""
    with open(file_path, "r", encoding="utf-8") as f:
        return sum(1 for _ in f)

def process_log_file(log_file):
    """Processa o arquivo de log, separando por data e removendo mensagens inúteis, com barra de progresso."""
    if not os.path.exists(log_file):
        print(f"Arquivo {log_file} não encontrado.")
        return

    total_lines = count_lines(log_file)
    processed_lines = 0

    temp_sensor_name = log_file.split("/")[-1].split(".")[0]
    output_dir = os.path.join(os.path.dirname(log_file), temp_sensor_name)
    os.makedirs(output_dir, exist_ok=True)

    with open(log_file, "r", encoding="utf-8") as f:
        for line in f:
            processed_lines += 1
            progress = (processed_lines / total_lines) * 100
            sys.stdout.write(f"\rProcessando: {progress:.2f}% ({processed_lines}/{total_lines})")
            sys.stdout.flush()

            if should_ignore(line):
                continue  # Pula logs irrelevantes

            match = re.match(r"^(\d{4}-\d{2}-\d{2})", line)
            if match:
                log_date = match.group(1)
                output_file = os.path.join(output_dir, f"{log_date}.log")

                with open(output_file, "a", encoding="utf-8") as out_f:
                    out_f.write(line)  # Mantém a linha original

    print("\nLogs processados! Arquivos salvos em:", output_dir)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python clean_logs.py <arquivo_de_log>")
    else:
        if sys.argv[1] == ".":
            for file in os.listdir():
                if file.endswith(".log"):
                    process_log_file(file)
        else:
            process_log_file(sys.argv[1])

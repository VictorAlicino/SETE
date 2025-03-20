import serial
from datetime import datetime

def find_message(data):
    hex_data = ''.join(f'{byte:02X}' for byte in data)  # Converte para string hex
    start_seq = "FFEEDD"
    end_seq = "DDEEFF"
    
    start_idx = hex_data.find(start_seq)
    end_idx = hex_data.find(end_seq, start_idx + len(start_seq))
    
    if start_idx != -1 and end_idx != -1:
        return hex_data[start_idx:end_idx + len(end_seq)]
    return None

def main():
    port = input("Digite a porta serial (ex: COM3 ou /dev/ttyUSB0): ")
    try:
        ser = serial.Serial(port, 9600, timeout=1)
        print(f"Monitorando {port} a 9600bps...")
        buffer = bytearray()
        # Send a message to start the communication
        b = bytes.fromhex("FFEEDD000209010A")
        ser.write(b)
        
        while True:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)  # Lê os bytes disponíveis
                buffer.extend(data)
                
                message = find_message(buffer)
                if message:
                    print(f"{datetime.now()} Mensagem detectada: {message}")
                    buffer.clear()  # Limpa buffer após detectar mensagem
    
    except serial.SerialException as e:
        print(f"Erro ao abrir a porta serial: {e}")
    except KeyboardInterrupt:
        print("\nEncerrando...")
        ser.close()

if __name__ == "__main__":
    main()

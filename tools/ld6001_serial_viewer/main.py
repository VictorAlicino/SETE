import serial
from datetime import datetime

def parse_frame(data):
    hex_data = ''.join(f'{byte:02X}' for byte in data)

    start_seq = "0102030405060708"
    start_idx = hex_data.find(start_seq)

    if start_idx == -1:
        return None, None

    # Procura pelo frame header (55AA) após a sequência inicial
    header_idx = hex_data.find("55AA", start_idx + len(start_seq))
    if header_idx == -1:
        return None, None

    try:
        frame_start_byte = header_idx // 2  # Cada byte tem 2 hex digits
        frame_length = data[frame_start_byte + 2]  # Número de bytes após "55 AA"
        frame_end_byte = frame_start_byte + 2 + frame_length  # Removido o checksum adicional

        if frame_end_byte > len(data):
            # Mensagem incompleta
            return None, None

        complete_frame = data[start_idx // 2:frame_end_byte]
        remaining_buffer = data[frame_end_byte:]
        return complete_frame, remaining_buffer

    except IndexError:
        # Mensagem incompleta
        return None, None

def main():
    port = input("Digite a porta serial (ex: COM3 ou /dev/ttyUSB0): ")
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"Monitorando {port} a 115200bps...")
        buffer = bytearray()
        ser.write(b"AT+DEBUG=2\n")
        
        while True:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)

                while True:
                    frame, new_buffer = parse_frame(buffer)
                    if frame:
                        print(f"{datetime.now()} Mensagem detectada: {' '.join(f'{b:02X}' for b in frame)}\n")
                        buffer = bytearray(new_buffer)
                    else:
                        break

    except serial.SerialException as e:
        print(f"Erro ao abrir a porta serial: {e}")
    except KeyboardInterrupt:
        print("\nEncerrando...")
        ser.close()

if __name__ == "__main__":
    main()
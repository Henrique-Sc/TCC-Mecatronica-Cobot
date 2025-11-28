import serial
import serial.tools.list_ports
import threading
import msvcrt
from time import sleep

# -----------------------------
# Seleção da porta
# -----------------------------

while True:
    ports = serial.tools.list_ports.comports()
    print('---- Lista de portas encontradas ----')
    for port in ports:
        if 'USB' in port.description:
            descricao = "Dispositivo Serial (USB)"
        elif 'Bluetooth' in port.description:
            descricao = "Dispositivo Serial (Bluetooth)"
        else:
            descricao = "Porta Serial"
        print(f"\033[31m{port.device}\033[m - {descricao}")

    porta = input('\nSelecione uma porta Serial ou \'ENTER\' para atualizar: ')
    print('')
    if porta != '':
        break

print(f'Conectando em {porta}...\n')

# -----------------------------
# Conexão serial
# -----------------------------
ser = serial.Serial(porta, 500000, timeout=0)
sleep(2)

# -----------------------------
# Thread para leitura contínua
# -----------------------------
def ler_serial():
    while True:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting).decode(errors="ignore")
            if data:
                print(f"\n<<< {data}", end="", flush=True)
        sleep(0.01)

threading.Thread(target=ler_serial, daemon=True).start()

# -----------------------------
# Terminal interativo
# -----------------------------
print("Terminal Serial iniciado! 🚀")
print("Digite comandos sem precisar apertar Enter.")
print("Use ESC para sair.\n")

buffer = ""
cursor = 0
historico = []
historico_index = -1

while True:
    if msvcrt.kbhit():
        char = msvcrt.getwch()

        # ESC para sair
        if char == '\x1b':
            print("\nEncerrando conexão...")
            ser.close()
            break

        # Enter envia comando
        elif char == '\r':
            print()
            if buffer.strip():
                ser.write((buffer + "\n").encode())
                print(f">>> {buffer}")
                historico.append(buffer)
            buffer = ""
            cursor = 0
            historico_index = len(historico)
        
        # Backspace
        elif char == '\b':
            if cursor > 0:
                buffer = buffer[:cursor-1] + buffer[cursor:]
                cursor -= 1
                # Move cursor para trás e limpa a letra
                print("\b \b", end="", flush=True)
                # Reescreve o restante do buffer
                print(buffer[cursor:] + " ", end="", flush=True)
                print("\b" * (len(buffer) - cursor + 1), end="", flush=True)

        # Teclas especiais (arrows)
        elif char == '\xe0':
            char2 = msvcrt.getwch()
            # Left arrow
            if char2 == 'K' and cursor > 0:
                cursor -= 1
                print("\b", end="", flush=True)
            # Right arrow
            elif char2 == 'M' and cursor < len(buffer):
                print(buffer[cursor], end="", flush=True)
                cursor += 1
            # Up arrow (histórico)
            elif char2 == 'H' and historico:
                if historico_index > 0:
                    historico_index -= 1
                    buffer = historico[historico_index]
                    cursor = len(buffer)
                    print("\r" + " " * 80 + "\r", end="", flush=True)  # limpa linha
                    print(buffer, end="", flush=True)
            # Down arrow (histórico)
            elif char2 == 'P' and historico:
                if historico_index < len(historico) - 1:
                    historico_index += 1
                    buffer = historico[historico_index]
                else:
                    historico_index = len(historico)
                    buffer = ""
                cursor = len(buffer)
                print("\r" + " " * 80 + "\r", end="", flush=True)  # limpa linha
                print(buffer, end="", flush=True)

        # Caracteres normais
        else:
            buffer = buffer[:cursor] + char + buffer[cursor:]
            cursor += 1
            print(buffer[cursor-1:], end="", flush=True)
            print("\b" * (len(buffer) - cursor), end="", flush=True)

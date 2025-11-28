# serial_comm.py
import serial
import serial.tools.list_ports
import threading
from time import sleep, time

class SerialManager:
    def __init__(self):
        """
        Inicializa o gerenciador de comunicação serial.
        """
        self.ser = serial.Serial()  # objeto Serial
        self._running = False
        self._buffer = ""           # guarda dados lidos (para comandos)
        self._listeners = []        # callbacks para log e interface

    # --- Sistema de callbacks ---
    def add_listener(self, callback):
        """Registra uma função que será chamada com cada linha lida."""
        self._listeners.append(callback)

    def _notify_listeners(self, data):
        for f in self._listeners:
            try:
                f(data)
            except Exception as e:
                print("Erro no listener:", e)

    @staticmethod
    def listar_portas():
        """
        Retorna uma lista de dicionários com portas seriais disponíveis.
        Exemplo:
        [
            {"porta": "COM3", "descricao": "Dispositivo Serial (USB)"},
            {"porta": "COM8", "descriao": "Dispositivo Serial (Bluetooth)"}
            {"porta": "COM4", "descricao": "Porta Serial"}
        ]
        """
        dispositivos = []
        for port in serial.tools.list_ports.comports():
            # descricao = f"({port.device}) " + " ".join(port.description.split(" ")[:-1]) or port.description
            if 'USB' in port.description:
                descricao = "Dispositivo Serial (USB)"
            elif 'Bluetooth' in port.description:
                descricao = "Dispositivo Serial (Bluetooth)"
            else:
                descricao = "Dispositivo Serial"
            dispositivos.append({
                "porta": port.device,
                "descricao": descricao
            })
        return dispositivos

     # --- Comunicação Serial ---
    def conectar(self, porta, baudrate=115200, timeout=0):
        if self.ser and self.ser.is_open:
            self.desconectar()
        self.ser = serial.Serial(porta, baudrate=baudrate, timeout=timeout)
        sleep(2)
        self._running = True
        threading.Thread(target=self._leitor_serial, daemon=True).start()

    def _leitor_serial(self):
        """Thread contínua que lê e armazena dados sem perder bytes ou quebrar linhas."""
        line_buffer = ""  # acumula até encontrar '\n'
        while self._running:
            if self.ser and self.ser.is_open and self.ser.in_waiting:
                try:
                    data = self.ser.read(self.ser.in_waiting).decode(errors="ignore")
                    if data:
                        self._buffer += data  # mantém compatibilidade com enviar_e_aguardar
                        line_buffer += data

                        # processa linhas completas
                        while "\n" in line_buffer:
                            linha, line_buffer = line_buffer.split("\n", 1)
                            linha = linha.strip("\r")
                            if linha:
                                # notifica os listeners (ex: log em UI)
                                self._notify_listeners(linha)
                                # imprime no terminal, se estiver rodando no modo direto
                                # if __name__ == "__main__":
                                #     print(f"<<< {linha}")
                except Exception as e:
                    print("Erro na leitura serial:", e)

            sleep(0.001)


    def ler_buffer(self):
        """Lê o buffer acumulado (sem apagar)."""
        return self._buffer

    def limpar_buffer(self):
        """Limpa o buffer interno."""
        self._buffer = ""

    def enviar(self, msg: str):
        if self.ser and self.ser.is_open:
            self.ser.write((msg + "\n").encode())

    def enviar_e_aguardar(self, comando, start="#", end="#", timeout=1.0):
        """Usa o buffer interno sem interferir na thread."""
        if not (self.ser and self.ser.is_open):
            return None

        self.limpar_buffer()
        self.enviar(comando)

        start_time = time() 
        while time() - start_time < timeout:
            data = self.ler_buffer()
            if start in data and end in data:
                ini = data.find(start)
                fim = data.find(end, ini + 1)
                if fim != -1:
                    return data[ini + 1:fim]
            sleep(0.001)
        return None

    def desconectar(self):
        self._running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
    
    def testar_comunicacao(self):
        """
        Verifica se a conexão está aberta e se a porta ainda está listada entre as disponíveis.
        """
        if self.ser and self.ser.is_open:
            portas_disponiveis = [p["porta"] for p in self.listar_portas()]
            return self.ser.port in portas_disponiveis
        return False


if __name__ == '__main__':
    import msvcrt
    from time import sleep

    def log_serial(data:str):
        """Callback para exibir os dados recebidos no terminal."""
        print(f"<<< {data}", end="\n", flush=True)

    # -----------------------------
    # Seleção da porta
    # -----------------------------
    while True:
        portas = SerialManager.listar_portas()
        print('---- Lista de portas encontradas ----')
        for p in portas:
            print(f"\033[31m{p['porta']}\033[m - {p['descricao']}")

        porta = input('\nSelecione uma porta Serial ou \'ENTER\' para atualizar: ')
        print('')
        if porta != '':
            break

    print(f'Conectando em {porta}...\n')

    # -----------------------------
    # Conexão SerialManager
    # -----------------------------
    ser_manager = SerialManager()
    ser_manager.add_listener(log_serial)
    ser_manager.conectar(porta, baudrate=115200)
    sleep(2)

    # # -----------------------------
    # # Terminal interativo
    # # -----------------------------
    # print("Terminal Serial iniciado! 🚀")
    # print("Digite comandos sem precisar apertar Enter.")
    # print("Use ESC para sair.\n")

    # buffer = ""
    # cursor = 0
    # historico = []
    # historico_index = -1

    # while True:
    #     if msvcrt.kbhit():
    #         char = msvcrt.getwch()

    #         # ESC para sair
    #         if char == '\x1b':
    #             print("\nEncerrando conexão...")
    #             ser_manager.desconectar()
    #             break

    #         # Enter envia comando
    #         elif char == '\r':
    #             print()
    #             if buffer.strip():
    #                 ser_manager.enviar(buffer)
    #                 print(f">>> {buffer}")
    #                 historico.append(buffer)
    #             buffer = ""
    #             cursor = 0
    #             historico_index = len(historico)
            
    #         # Backspace
    #         elif char == '\b':
    #             if cursor > 0:
    #                 buffer = buffer[:cursor-1] + buffer[cursor:]
    #                 cursor -= 1
    #                 print("\b \b", end="", flush=True)
    #                 print(buffer[cursor:] + " ", end="", flush=True)
    #                 print("\b" * (len(buffer) - cursor + 1), end="", flush=True)

    #         # Teclas especiais (arrows)
    #         elif char == '\xe0':
    #             char2 = msvcrt.getwch()
    #             # Left arrow
    #             if char2 == 'K' and cursor > 0:
    #                 cursor -= 1
    #                 print("\b", end="", flush=True)
    #             # Right arrow
    #             elif char2 == 'M' and cursor < len(buffer):
    #                 print(buffer[cursor], end="", flush=True)
    #                 cursor += 1
    #             # Up arrow (histórico)
    #             elif char2 == 'H' and historico:
    #                 if historico_index > 0:
    #                     historico_index -= 1
    #                     buffer = historico[historico_index]
    #                     cursor = len(buffer)
    #                     print("\r" + " " * 80 + "\r", end="", flush=True)
    #                     print(buffer, end="", flush=True)
    #             # Down arrow (histórico)
    #             elif char2 == 'P' and historico:
    #                 if historico_index < len(historico) - 1:
    #                     historico_index += 1
    #                     buffer = historico[historico_index]
    #                 else:
    #                     historico_index = len(historico)
    #                     buffer = ""
    #                 cursor = len(buffer)
    #                 print("\r" + " " * 80 + "\r", end="", flush=True)
    #                 print(buffer, end="", flush=True)

    #         # Caracteres normais
    #         else:
    #             buffer = buffer[:cursor] + char + buffer[cursor:]
    #             cursor += 1
    #             print(buffer[cursor-1:], end="", flush=True)
    #             print("\b" * (len(buffer) - cursor), end="", flush=True)

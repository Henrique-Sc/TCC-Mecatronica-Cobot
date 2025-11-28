import threading
import time
import json
import webview 
from serial_comm import SerialManager  # módulo para comunicação serial
from controllers import Cobot
from pathlib import Path

# ======================
# Inicialização do app
# ======================
def run_app():
   base_dir = Path(__file__).parent
   html_dir = base_dir / 'web'  

   class Api ():
      def __init__(self):
         self._ser = SerialManager()
         self.cobot = Cobot(self._ser)

         self._ser.add_listener(self._log_serial)
         self._ser.add_listener(self._monitorar_steps)
         self._ser.add_listener(self._monitorar_savepos)
         self._ser.add_listener(self._monitorar_jointpos)

         self.ultima_lista = []
         self.porta = None
         self.conectado = False


      def _resposta(self, sucesso, mensagem, dados={}):
         return {
            "sucesso": sucesso,
            "mensagem": mensagem,
            "dados": dados,
            "timestamp": time.time()
         }
      
      def _log_serial(self, data:str):
        """Callback para exibir os dados recebidos no terminal."""
        print(f"<<< {data}", end="\n", flush=True)
      
      def _monitorar_steps(self, string:str):
         if "#STEP" in string:
            step = dict([string.replace('#', '').lower().split(':')])
            window.evaluate_js(f"setStep('{json.dumps(step)}')")
            print(step)
      
      def _monitorar_savepos(self, string:str):
         if "#SAVEPOS#" in string:
            self.add_point()
            window.evaluate_js('setPoints();')
      
      def _monitorar_jointpos(self, string:str):
         if ("#J" in string):
            joint, pos = string.replace('#', '').split(':')
            joint = int(joint[1]) - 1
            if (joint < 5):
               pos = int(pos)
               self.cobot.set_joint_pos(joint, pos)
               window.evaluate_js(f"setJointPos({joint}, {pos})")

      # ======================
      # Monitoramento de portas
      # ======================
      def listar_portas(self):
         """
         Retorna a lista de dispositivos seriais conectados
         para o frontend em formato de lista de dicionários.
         """
         try:
            lista = self._ser.listar_portas()
            return self._resposta(True, "Lista obtida com sucesso", lista)
         except Exception as e:
            return self._resposta(False, f"Erro ao listar portas: {str(e)}")
      
      def testar_comunicacao(self):
         try:
            resp = self._ser.testar_comunicacao()
            return self._resposta(resp, "Comunicação OK" if resp else "Não foi possível comunicar com a porta")
         except Exception as e:
            return self._resposta(False, f"Erro: {str(e)}")

      def start_monitor(self, interval=1):
         # lista = self._ser.listar_portas()
         # print(json.dumps(lista))
         # window.evaluate_js(f'console.log({json.dumps(lista)})')
         def monitor():
            while True:
                lista = self._ser.listar_portas()
                if lista != self.ultima_lista:
                  self.ultima_lista = lista
                  # converte para JSON e chama JS
                  # print()
                  js_code = f"carregarPortas({json.dumps(lista)}); testarConexao()"  # testarConexao()
                  window.evaluate_js(f"{js_code}") # type: ignore
                time.sleep(interval)  # ajusta intervalo conforme necessário

         threading.Thread(target=monitor, daemon=True).start()
         return self._resposta(True, "Monitoramento serial iniciado", {"interval": interval})

      # ======================
      # Conexão e desconexão
      # ======================
      def conectar(self, porta, baudrate=115200, timeout=0):
         """Abre conexão serial"""
         try:
            self._ser.conectar(porta, baudrate, timeout)
            self.conectado = True
            self.porta = porta

            # def ler_serial():
            #    while True:
            #          if self._ser.ser.in_waiting:
            #             data = self._ser.ler()
            #             if data:
            #                print(f"\n<<< {data}", end="", flush=True)
            #          time.sleep(0.01)

            # threading.Thread(target=ler_serial, daemon=True).start()
            self.enviar('M:P')
            return self._resposta(True, f"Conectado à porta {porta} com {baudrate}bps")
         except Exception as e:
            self.conectado = False
            return self._resposta(False, f"Falha ao conectar: {str(e)}")
         
      def desconectar(self):
         """Fecha conexão serial"""
         try:
            self._ser.desconectar()
            self.conectado = False
            return self._resposta(True, f"Porta desconectada com sucesso")
         except Exception as e:
            return self._resposta(False, f"Falha ao desconectar: {str(e)}")
         
      # ======================
      # Envio e leitura
      # ======================
      def enviar(self, msg: str):
         """Envia comando ao dispositivo"""
         if not self.conectado:
            return self._resposta(False, f"Nenhuma porta conectada")
               
         try:
            self._ser.enviar(msg)
            return self._resposta(True, "Dado enviado com sucesso")
         
         except Exception as e:
            return self._resposta(False, f"Erro ao enviar: {str(e)}")
      
      def add_point(self, pos=None):
         self.cobot.add_point(pos)
         window.evaluate_js('setPoints();')
         
      def delete_point(self, i):
         self.cobot.delete_point(i)
         window.evaluate_js('setPoints();')

      def get_points(self):
         return self.cobot.points
      
      def clear_points(self):
         self.cobot.clear_point()
         window.evaluate_js('setPoints();')
      
      def save_program(self, name, dom, id=None):
        return self.cobot.save_program(name, dom, id)

      def load_program(self, id):
         return self.cobot.load_program(id)

      def load_slots(self):
         return self.cobot.load_slots()

      def delete_slot(self, id):
         return self.cobot.delete_slot(id)

      def rename_slot(self, id, name):
         return self.cobot.update_slot_name(id, name)
      

   api = Api()
   window = webview.create_window(
      'Cobot',
      url=f'file://{html_dir}/index.html',
      js_api=api,
      min_size=(1212, 726)
   )
   webview.start(debug=True)


if __name__ == '__main__':
   run_app()
   
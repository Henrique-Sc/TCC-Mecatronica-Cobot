from serial_comm import SerialManager
from time import sleep
import json
import uuid
from pathlib import Path


class Cobot:
    def __init__(self, ser: SerialManager):
        self._ser = ser
        self.points = []
        self.joint_pos = [90, 30, 152, 90, 84]

        self._saves_path = Path('app/saves')
        self._saves_path.mkdir(parents=True, exist_ok=True)

        self._slots_file = self._saves_path / 'slots.json'
        self.slots = []

        self._load_slots()


    # ============================================================
    # === LOAD E SALVAR SLOTS ====================================
    # ============================================================

    def _load_slots(self):
        """Carrega a lista de slots."""
        if not self._slots_file.exists():
            self._slots_file.write_text("[]", encoding="utf-8")

        try:
            conteudo = self._slots_file.read_text(encoding="utf-8").strip()
            self.slots = json.loads(conteudo) if conteudo else []
        except Exception as e:
            print("ERRO NA LEITURA DOS SAVES\n", e)
            self.slots = []
            self._save_slots()

    def _save_slots(self):
        """Atualiza o arquivo slots.json."""
        self._slots_file.write_text(
            json.dumps(self.slots, indent=2),
            encoding="utf-8"
        )


    # ============================================================
    # === GERENCIAMENTO DE IDS ===================================
    # ============================================================

    def gerar_id(self):
        existente = {item["id"] for item in self.slots}

        while True:
            novo_id = uuid.uuid4().hex
            if novo_id not in existente:
                return novo_id


    # ============================================================
    # === CRUD DE SLOTS (CREATE, READ, UPDATE, DELETE) ============
    # ============================================================

    def create_slot(self, name: str, dom: str):
        """Cria um save novo (arquivo e registro no slots.json)."""
        new_id = self.gerar_id()

        # salva conteúdo do DOM
        dados = json.loads(dom)
        (self._saves_path / f"{new_id}.json").write_text(
            json.dumps(dados, indent=2),
            encoding="utf-8"
        )

        # grava na lista de slots
        self.slots.append({"id": new_id, "name": name})
        self._save_slots()

        return new_id


    def update_slot_name(self, id: str, new_name: str):
        """Atualiza o nome apenas dentro do slots.json."""
        for slot in self.slots:
            if slot["id"] == id:
                slot["name"] = new_name
                self._save_slots()
                return True
        return False


    def delete_slot(self, id: str):
        """Remove o arquivo <id>.json e o slot correspondente."""
        # remove arquivo físico
        path = self._saves_path / f"{id}.json"
        if path.exists():
            path.unlink()

        # remove do array
        original = len(self.slots)
        self.slots = [slot for slot in self.slots if slot["id"] != id]

        # salva arquivo atualizado
        self._save_slots()

        return len(self.slots) < original


    # ============================================================
    # === SALVAR / ATUALIZAR UM PROGRAMA =========================
    # ============================================================

    def save_program(self, name: str, dom: str, id=None):
        """
        Salva ou atualiza um programa.
        - Cria novo se o id for None ou não existir.
        - Atualiza conteúdo e nome se já existir.
        """
        ids_existentes = {s["id"] for s in self.slots}

        if id is None or id not in ids_existentes:
            return self.create_slot(name, dom)

        # atualizar arquivo
        dados = json.loads(dom)
        (self._saves_path / f"{id}.json").write_text(
            json.dumps(dados, indent=2),
            encoding="utf-8"
        )

        # atualizar nome
        self.update_slot_name(id, name)

        return id


    # ============================================================
    # === LOAD DE DADOS ==========================================
    # ============================================================

    def load_slots(self):
        """Retorna a lista de slots (id + name)."""
        return self.slots


    def load_program(self, id: str):
        """Retorna o JSON do arquivo <id>.json como string."""
        path = self._saves_path / f"{id}.json"

        if not path.exists():
            return "[]"

        try:
            program = json.loads(path.read_text(encoding="utf-8"))
            self.clear_point()
            if (program):
                for point in program['points'].values():
                    self.add_point(point)

            return program
        except Exception as erro:
            print('ERRO AO CARREGAR PROGRAMA. ')
            print(erro)
            return "[]"


    # ============================================================
    # === FUNÇÕES DO ROBÔ ========================================
    # ============================================================

    def add_point(self, pos=None):
        self.points.append(self.joint_pos[:] if pos is None else pos)

    def delete_point(self, i):
        if 0 <= i < len(self.points):
            self.points.pop(i)

    def set_points(self, points):
        self.points, points

    def clear_point(self):
        self.points.clear()

    def get_joint_pos(self, j):
        return self.joint_pos[j]

    def set_joint_pos(self, j, p):
        self.joint_pos[j] = p


#include "config.h"
#include "servo.cpp"
#include <Wire.h>
#include <EEPROM.h>


#ifdef ESP32
#include <Adafruit_PWMServoDriver.h>
// #include <BluetoothSerial.h>
// BluetoothSerial SerialBT;
#else
#include <avr/wdt.h>

class FakePWM {
public:
  void begin() {
    serialPrintln("FakePWM initialized");
  }

  void setPWMFreq(float freq) {
    serialPrintln("Set PWM freq: ", freq);
  }

  void setPWM(uint8_t channel, uint16_t on, uint16_t off) {
    if (Serial) {
      // serialPrintln("Channel ", channel, " -> On: ", on, ", Off: ", off);
    } else {
      // serialPrintln("Serial não inicializado.");
    }
  }
};
#endif

enum class CmdType : uint8_t {
  MOVE,
  MOVESMOOTH,
  WAIT,
  GRIP
};

struct Cmd {
  CmdType type;
  uint8_t move[5];     // ângulos das juntas
  uint8_t speed = 10;  // velocidade para Move / MoveSmooth / Joint
  uint16_t wait = 0;   // tempo de espera para Wait
  bool grip = 0;       // 0 - aberta; 1 - fechada

  void dump() {

    if (type == CmdType::MOVE) {
      serialPrint("Move | Vel: ", speed, " | Pos: ");
      printArray(move);
      serialPrintln("");
    }
    if (type == CmdType::MOVESMOOTH) {
      serialPrint("MoveSmooth | Vel: ", speed, " | Pos: ");
      printArray(move);
      serialPrintln("");
    }
    if (type == CmdType::WAIT) {
      serialPrintln("Wait | Tempo: ", wait, "ms");
    }
    if (type == CmdType::GRIP) {
      serialPrintln("Grip | Acionar: ", grip);
    }
  }
};


// ---- Gerenciador EEPROM ----

class EepromManager {
public:
  static constexpr size_t calibBytesPerJoint =
    sizeof(uint8_t) + BASE_MESH_SIZE * sizeof(int16_t);
  static constexpr size_t totalCalibBytes =
    calibBytesPerJoint * NUM_JOINTS;

  static constexpr size_t sequenceBytesPerSlot =
    MAX_STEPS * sizeof(Cmd);
  static constexpr size_t totalSequenceBytes =
    sequenceBytesPerSlot * MAX_SEQUENCES;

  static constexpr size_t startCalib = 0;
  static constexpr size_t startSequences = startCalib + totalCalibBytes;

  void begin() {
    EEPROM.begin(EEPROM_SIZE);
  }

  // --- Calibração ---

  void saveCalibration(uint8_t jointIndex, Servo<Adafruit_PWMServoDriver>* servo) {
    size_t addr = startCalib + jointIndex * calibBytesPerJoint;

    // obtém dados do servo
    uint8_t numPoints = servo->getNumCalibPoints();
    const uint16_t* feedback = servo->getCalibFeedback();

    // salva número de pontos
    EEPROM.put(addr, numPoints);
    addr += sizeof(uint8_t);

    // salva os valores de feedback
    for (uint8_t i = 0; i < BASE_MESH_SIZE; i++) {
      EEPROM.put(addr, feedback[i]);
      addr += sizeof(uint16_t);
    }

    EEPROM.commit();
  }

  void loadCalibration(uint8_t jointIndex, Servo<Adafruit_PWMServoDriver>* servo) {
    size_t addr = startCalib + jointIndex * calibBytesPerJoint;

    uint8_t numPoints;
    uint16_t feedback[BASE_MESH_SIZE];

    EEPROM.get(addr, numPoints);
    addr += sizeof(uint8_t);

    for (uint8_t i = 0; i < BASE_MESH_SIZE; i++) {
      EEPROM.get(addr, feedback[i]);
      addr += sizeof(uint16_t);
    }

    // aplica os dados no servo
    // serialPrintln("Num calib points: ", numPoints);
    // serialPrintln("Calib points: ");
    // for (int c=0; c < numPoints; c++) {
    //   serialPrintln("    Ponto ", c, " -> ", feedback[c]);
    // }

    servo->setNumCalibPoints(numPoints);
    servo->setCalibFeedback(feedback);
  }

  // --- Sequências ---

  void saveSequence(uint8_t slot, uint8_t numSteps, Cmd sequence[MAX_STEPS]) {
    size_t addr = startSequences + slot * sequenceBytesPerSlot;
    EEPROM.put(addr, numSteps);  // salva quantos passos são válidos
    addr += sizeof(uint8_t);

    for (uint8_t i = 0; i < numSteps; i++) {  // só salva os passos úteis
      EEPROM.put(addr, sequence[i]);
      addr += sizeof(Cmd);
    }
    EEPROM.commit();
  }

  void loadSequence(uint8_t slot, uint8_t& numSteps, Cmd sequence[MAX_STEPS]) {
    size_t addr = startSequences + slot * sequenceBytesPerSlot;
    EEPROM.get(addr, numSteps);
    addr += sizeof(uint8_t);

    for (uint8_t i = 0; i < numSteps; i++) {  // só carrega os passos válidos
      EEPROM.get(addr, sequence[i]);
      addr += sizeof(Cmd);
    }
  }

  void debugLayout() {
    serialPrintln("==== EEPROM Layout ====");
    serialPrintln("Joint calib per joint: ", calibBytesPerJoint);
    serialPrintln("Total calib bytes: ", totalCalibBytes);
    serialPrintln("Sequence per slot: ", sequenceBytesPerSlot);
    serialPrintln("Total sequence bytes: ", totalSequenceBytes);
    serialPrintln("Total used: ", startSequences + totalSequenceBytes);
    serialPrintln("=======================");
  }
};


// --- CRIANDO OBJETOS ---
#ifdef ESP32
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Servo<Adafruit_PWMServoDriver> servos[] = {
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_0_CHANNEL, 1, FEEDBACK_0_PIN, 0, 180, 90),
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_1_CHANNEL, 2, FEEDBACK_1_PIN, 0, 180, 0),
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_2_CHANNEL, 2, FEEDBACK_2_PIN, 0, 180, 180),
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_3_CHANNEL, 3, FEEDBACK_3_PIN, 0, 180, 180),
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_4_CHANNEL, 4, FEEDBACK_4_PIN, 0, 180, 90),
  // SERVO_5_CHANNEL deletado
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_6_CHANNEL, 5, FEEDBACK_6_PIN, 0, 180, 84),
  Servo<Adafruit_PWMServoDriver>(pwm, SERVO_7_CHANNEL, 6, FEEDBACK_7_PIN, 26, 110, 40),
};

Servo<Adafruit_PWMServoDriver>* joint[] = {
#else
FakePWM pwm;

Servo<FakePWM> servos[] = {
  Servo<FakePWM>(pwm, SERVO_0_CHANNEL, 1, FEEDBACK_0_PIN, 0, 180, 90),
  Servo<FakePWM>(pwm, SERVO_1_CHANNEL, 2, FEEDBACK_1_PIN, 0, 180, 0),
  Servo<FakePWM>(pwm, SERVO_2_CHANNEL, 2, FEEDBACK_2_PIN, 0, 180, 180),
  Servo<FakePWM>(pwm, SERVO_3_CHANNEL, 3, FEEDBACK_3_PIN, 0, 180, 180),
  Servo<FakePWM>(pwm, SERVO_4_CHANNEL, 4, FEEDBACK_4_PIN, 0, 180, 90),
  // SERVO_5_CHANNEL deletado
  Servo<FakePWM>(pwm, SERVO_6_CHANNEL, 5, FEEDBACK_6_PIN, 0, 180, 84),
  Servo<FakePWM>(pwm, SERVO_7_CHANNEL, 6, FEEDBACK_7_PIN, 26, 110, 40),
};

Servo<FakePWM>* joint[] = {
#endif
  &servos[0],  // Junta 1
  &servos[1],  // Junta 2
  &servos[3],  // Junta 3
  &servos[4],  // Junta 4
  &servos[5],  // Junta 5
  &servos[6],  // Junta 6
};

EepromManager Eeprom;


// --- CRIANDO VARIÁVEIS ---

Cmd sequence[MAX_STEPS];
uint8_t numSteps = 0;

uint8_t currentStep = 0;
bool stepStarted = false;
bool executingSequence = false;
bool clientConnected = false;
bool loopSequence = false;

unsigned long lastMoveEnd = 0;
unsigned long lastMoveGrip = 0;
unsigned long waitStart = 0;
const unsigned long MOVE_DELAY = 150;  // tempo fixo entre movimentos (em ms)

bool gripReached = false;
bool startBeginning = true;
bool initialPrint = true;
bool gripSituation = false;

bool btn1Ctrl = false;
bool btn2Ctrl = false;
bool btn3Ctrl = false;
unsigned long btn1T = 0;
unsigned long btn2T = 0;
unsigned long btn3T = 0;
const unsigned long DEBOUNCE = 200;  // 150 ms delay p/ botão

bool detachSituation = false;

// ========================================
//             --- SETUP ---
// ========================================
void setup() {
  // put your setup code here, to run once:

  pinMode(BTN_1, INPUT_PULLDOWN);
  pinMode(BTN_2, INPUT_PULLDOWN);
  pinMode(BTN_3, INPUT_PULLDOWN);

#ifdef ESP32
  Serial.begin(500000);
// SerialBT.begin("Cobot");
#else
  Serial.begin(115200);
#endif

  delay(1000);
  serialPrintln("\n===========================================\n");

  Eeprom.begin();
  Eeprom.debugLayout();  // Mostra os offsets e tamanhos calculados


#ifdef ESP32
  Wire.begin(21, 22);
#else
  Wire.begin();
#endif

  delay(50);

  pwm.begin();
  delay(10);

  pwm.setPWMFreq(50);
  delay(10);

  for (int c = 0; c < 15; c++) {
    pwm.setPWM(c, 0, 0);
  }

  servos[1].setMirror(&servos[2]);
  delay(500);

  for (auto* j : joint) {
    j->generateCalibMesh();  // agora o Serial já está iniciado
  }

  delay(500);

  servos[0].attach();
  delay(500);
  servos[1].attach();
  delay(500);
  servos[2].attach();
  delay(500);
  servos[3].attach();
  delay(500);
  servos[4].attach();
  delay(500);
  servos[5].attach();

  delay(2500);
  servos[6].attach();
  serialPrintln("\nHoming.");
  joint[0]->move(90, 1);
  joint[1]->move(30, 1);
  joint[2]->move(152, 1);
  joint[3]->move(90, 1);
  joint[4]->move(82, 1);
  abrirGarra();
  while (true) {
    bool finish = true;
    for (int c = 0; c < 6; c++) {
      joint[c]->update();
      if (joint[c]->isMoving()) {
        finish = false;
      }
    }
    if (finish) {
      break;
    }
  }

  Eeprom.loadCalibration(0, joint[0]);
  Eeprom.loadCalibration(1, joint[1]);
  Eeprom.loadCalibration(2, joint[2]);
  Eeprom.loadCalibration(3, joint[3]);
  Eeprom.loadCalibration(4, joint[4]);
}


// ========================================
//             --- LOOP ---
// ========================================
void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    serialPrintln("========================================");
    serialPrintln("--- Resposta Serial Identificada ---");
    // Serial.setTimeout(2000);
    String serialInput = Serial.readStringUntil('\n');
    serialInput.trim();

    serialPrint("String: ");
    serialPrintln(serialInput);

    processInput(serialInput);

    serialPrintln("========================================\n");
    serialPrintln("DONE");
  }

  // #ifdef ESP32
  // if (SerialBT.available()) {
  //   serialPrintln("========================================");
  //   serialPrintln("--- Resposta Serial Identificada ---");
  //   // Serial.setTimeout(2000);
  //   String serialInputBT = SerialBT.readStringUntil('\n');
  //   serialInputBT.trim();

  //   serialPrint("String: ");
  //   serialPrintln(serialInputBT);

  //   processInput(serialInputBT);

  //   serialPrintln("========================================\n");
  // }
  // if (SerialBT.hasClient()) {
  //   // Exibir dispositivo conectado
  //   if (!clientConnected) {
  //     clientConnected = true;
  //     serialPrintln("Dispositivo conectado!");
  //   }
  // } else clientConnected = false;
  // #endif

  servos[0].update();
  servos[1].update();
  // Servo espelhadoooo -> servos[2].update();
  servos[3].update();
  servos[4].update();
  servos[5].update();
  servos[6].update();


  if (executingSequence) executeSequence();

  if (!joint[5]->isMoving() && !gripReached) {       // Finalizou o movimento e não finalizou a garra (Var de controle)
    if (lastMoveGrip == 0) lastMoveGrip = millis();  // Criar um init para o millis
    if (millis() - lastMoveGrip >= 100) {            // Delay de 100ms para a garra entrar na posição de relax mecânico
      // Na posição máxima, a posição de relaxamento é 94°
      if (!gripSituation) {  // Se a posição foi para abrir...
        joint[5]->detach();  // Relaxa o servo
      }
      // Não existe ainda alguma posição de relaxamento quando está fechada

      // Marcar como finalizado, e resetar variavel de controle do millis
      gripReached = true;
      lastMoveGrip = 0;
    }
  }

  if (!digitalRead(BTN_1)) btn1Ctrl = false;
  if (!digitalRead(BTN_2)) btn2Ctrl = false;
  if (!digitalRead(BTN_3)) btn3Ctrl = false;

  if (digitalRead(BTN_1) && !btn1Ctrl && millis() - btn1T > DEBOUNCE) {
    // serialPrintln("Botão 1 pressionado");
    btn1Ctrl = true;
    btn1T = millis();

    detachSituation = !detachSituation;
    if (detachSituation) {
      for (int c = 0; c <= 4; c++) {
        joint[c]->detach();
      }
    } else {
      for (int c = 0; c <= 4; c++) {
        joint[c]->setAngle(joint[c]->getFeedback());
        joint[c]->attach();
      }
    }
  }
  if (digitalRead(BTN_2) && !btn2Ctrl && millis() - btn2T > DEBOUNCE) {
    // serialPrintln("Botão 2 pressionado");
    btn2Ctrl = true;
    btn2T = millis();

    if (gripSituation) abrirGarra();
    else fecharGarra();
  }
  if (digitalRead(BTN_3) && !btn3Ctrl && millis() - btn3T > DEBOUNCE) {
    // serialPrintln("Botão 3 pressionado");
    btn3Ctrl = true;
    btn3T = millis();

    serialPrintln("#SAVEPOS#");
  }
}



// ========================================
//             --- FUNÇÕES ---
// ========================================
void processInput(String input) {
  input.trim();

  String data[4];
  uint8_t data_idx = 0;

  for (int i = 0; i < input.length(); i++) {
    if (input[i] == ':' && data_idx < 3) data_idx++;
    else {
      data[data_idx] += input[i];
    }
  }

  processCommand(data);
}

void processCommand(String data[4]) {
  if (data[0][0] == 'S') {
    uint8_t target = data[0][1] - '0' - 1;
    if (data[1] == "F") {
      serialPrintln("setPwm(", data[2], ")");
      servos[target].setPwm(data[2].toInt());
    }
  }
  if (data[0][0] == 'J') {
    uint8_t target = data[0][1] - '0' - 1;

    if (data[1] == "A") {
      serialPrintln("attach()");
      joint[target]->attach();
    }

    if (data[1] == "D") {
      serialPrintln("detach()");
      joint[target]->detach();
    }

    if (data[1] == "W") {
      serialPrintln("write(", data[2], ")");
      joint[target]->write(data[2].toInt());
    }
    if (data[1] == "F") {
      serialPrintln("setPwm(", data[2], ")");
      joint[target]->setPwm(data[2].toInt());
    }
    if (data[1] == "M") {
      serialPrintln("move(", data[2], ", ", data[3].toInt() <= 0 ? 10 : data[3].toInt(), ")");
      joint[target]->move(data[2].toInt(), data[3].toInt() <= 0 ? 10 : data[3].toInt());
    }
    if (data[1] == "MS") {
      serialPrintln("moveSmooth(", data[2], ", ", data[3].toInt() <= 0 ? 10 : data[3].toInt(), ")");
      joint[target]->moveSmooth(data[2].toInt(), data[3].toInt() <= 0 ? 10 : data[3].toInt());
    }
    if (data[1] == "S") {
      serialPrintln("Step(", data[2].toInt(), ")");
      joint[target]->step(data[2].toInt());
    }
    if (data[1] == "L") {
      joint[target]->setLog(data[2] == "1");
    }
    if (data[1] == "LM") {
      joint[target]->setLogInMove(data[2] == "1");
    }
    if (data[1] == "C") {
      joint[target]->calibrateFeedbackMultiPoint();
    }
  }

  if (data[0] == "G") {
    if (data[1] == "1") {
      serialPrintln("Fechar garra");
      fecharGarra();
    } else {
      serialPrintln("Abrir garra");
      abrirGarra();
    }
  }

  if (data[0] == "M") {
    if (data[1] == "A") {
      serialPrintln("all->attach()");
      for (int c = 0; c < 6; c++) {
        joint[c]->attach();
        delay(600);
      }
    }
    if (data[1] == "D") {
      serialPrintln("all->detach()");
      for (int c = 0; c < 6; c++) {
        joint[c]->detach();
        delay(150);
      }
    }
    if (data[1] == "R") {
#ifdef ESP32
      serialPrintln("ESP.restart()");
      ESP.restart();
#else
      serialPrintln("Arduino Reset");
      wdt_enable(WDTO_15MS);
      while (1) {}
#endif
    }
    if (data[1] == "H") {
      serialPrintln("home()");
      joint[0]->move(90, 1);
      joint[1]->move(30, 1);
      joint[2]->move(152, 1);
      joint[3]->move(90, 1);
      joint[4]->move(84, 1);
      // joint[5]->move(45, 1);
    }
    if (data[1] == "C") {
      // joint[0]->calibrateFeedbackMultiPoint();
      // delay(200);
      // joint[1]->calibrateFeedbackMultiPoint();
      // delay(200);
      // joint[3]->calibrateFeedbackMultiPoint();
      // delay(200);
      // joint[4]->calibrateFeedbackMultiPoint();
      // delay(200);
      // joint[5]->calibrateFeedbackMultiPoint();
      // delay(200);
      // joint[6]->calibrateFeedbackMultiPoint();
    }
    if (data[1] == "P") {
      // String data = "#POS:";
      for (int i = 0; i < 6; i++) {
        int angle = joint[i]->getAngle();
        serialPrintln("#J", i + 1, ":", angle, "#");
        //   data += String(angle);
        //   if (i < 5) {
        //     data += ";";
        //   }
      }
      // serialPrintln(data + "#");
    }
    if (data[1] == "M" || data[1] == "MS") {  // M:M:10:#90;0;180;90;90;120#  ||  M:MS:6:#20;30;152;90;90;28#
      int angles[6] = { 0 };
      int count = parseAnglesFromHash(data[3], angles, 6);  // String, array, tamanho do array  ||  "#90;0;180;90;90;28#", angles[6], 6

      for (int c = 0; c < count; c++) {
        if (data[1] == "M") {
          // serialPrintln("joint[",c,"]->move(",angles[c],", ",data[2],")");
          joint[c]->move(angles[c], data[2].toInt());
        } else {
          // serialPrintln("joint[",c,"]->moveSmooth(",angles[c],", ",data[2],")");
          joint[c]->moveSmooth(angles[c], data[2].toInt());
        }
      }
    }
    if (data[1] == "LM") {
      for (int c = 0; c < 6; c++) {
        joint[c]->setLogInMove(data[2] == "1");
      }
    }
    if (data[1] == "SP") {
      serialPrintln("#SAVEPOS#");
    }
  }

  if (data[0] == "S") {  // Sequencia
    // ============================
    //  MOVE / MOVESMOOTH
    // ============================
    if (data[1] == "M" || data[1] == "MS") {
      if (numSteps >= MAX_STEPS) return;
      Cmd newCmd;

      if (data[1] == "M") newCmd.type = CmdType::MOVE;
      else newCmd.type = CmdType::MOVESMOOTH;

      // Velocidade (param 2)
      newCmd.speed = data[2].toInt();
      if (newCmd.speed <= 0) newCmd.speed = 10;

      // Verifica se há lista de ângulos (param 3)
      if (data[3] == "") {
        // Se não houver, usa posição atual do robô
        for (int i = 0; i < 5; i++) newCmd.move[i] = joint[i]->getAngle();
      } else {
        int angles[5] = { 0 };
        int count = parseAnglesFromHash(data[3], angles, 5);  // String, array, tamanho do array  ||  "#90;0;180;90;90;28#", angles[6], 6

        for (int c = 0; c < count; c++) newCmd.move[c] = angles[c];
      }

      // Adiciona o comando à sequência
      sequence[numSteps++] = newCmd;
      serialPrint(numSteps, " - ");
      newCmd.dump();
    }

    // ============================
    //  GRIP
    // ============================
    else if (data[1] == "G") {  // S:G:0 - S:G:1
      if (numSteps >= MAX_STEPS) return;
      Cmd newCmd;

      if (data[1] == "G") newCmd.type = CmdType::GRIP;
      newCmd.grip = data[2].toInt();

      // Adiciona o comando à sequência
      sequence[numSteps++] = newCmd;
      serialPrint(numSteps, " - ");
      newCmd.dump();
    }

    // ============================
    //  WAIT
    // ============================
    else if (data[1] == "W") {
      if (numSteps >= MAX_STEPS) return;
      Cmd newCmd;

      newCmd.type = CmdType::WAIT;
      newCmd.wait = data[2].toInt();
      if (newCmd.wait <= 0) newCmd.wait = 1000;  // padrão de 1000 ms

      sequence[numSteps++] = newCmd;
      serialPrint(numSteps, " - ");
      newCmd.dump();
    }

    // ============================
    //  PRINT
    // ============================
    if (data[1] == "P") {
      serialPrintln("Num steps: ", numSteps);
      serialPrintln("Current Step: ", currentStep);
      serialPrintln("Start Beginning: ", startBeginning);
      for (int i = 0; i < numSteps; i++) {
        Cmd c = sequence[i];
        serialPrint(i + 1, " - ");
        c.dump();
      }
    }

    // ============================
    //  CONTROLL
    // ============================
    if (data[1] == "C") {
      // CLEAR
      if (data[2] == "C") {
        numSteps = 0;
        executingSequence = false;
        currentStep = 0;
        startBeginning = true;
        initialPrint = true;
        serialPrintln("#STEP:INIT#");
      }

      // FAST BACKWARD
      if (data[2] == "B") {
        startBeginning = true;
        currentStep = 0;
        initialPrint = true;
        executingSequence = false;
        waitStart = 0;
        stepStarted = 0;
        serialPrintln("#STEP:INIT");
      }

      // FORWARD
      if (data[2] == "F") {
        executingSequence = false;
        if (!initialPrint && currentStep < numSteps) currentStep++;
        if (currentStep < numSteps) {
          serialPrintln("#STEP:" + String(currentStep + 1) + "#");

          Cmd c = sequence[currentStep];
          switch (c.type) {
            case CmdType::MOVE:
            case CmdType::MOVESMOOTH:
              for (int i = 0; i < 5; i++) {
                if (joint[i]->getAngle() != c.move[i]) {
                  if (c.type == CmdType::MOVE)
                    joint[i]->move(c.move[i], c.speed);
                  else
                    joint[i]->moveSmooth(c.move[i], c.speed);
                }
              }
              break;

            case CmdType::GRIP:
              if (c.grip) {
                fecharGarra();
              } else {
                abrirGarra();
              }
              break;
          }
        } else {
          serialPrintln("#STEP:END#");
          currentStep = constrain(currentStep, 0, numSteps - 1);
        }
        startBeginning = false;
        initialPrint = false;
      }

      // PAUSE
      if (data[2] == "P") {
        executingSequence = false;
        startBeginning = false;
      }

      // RUN
      if (data[2] == "R") {
        if (executingSequence) return;

        if (startBeginning) {
          currentStep = 0;
          stepStarted = false;
        }
        executingSequence = true;
      }

      if (data[2] == "L") {
        serialPrintln("Loop Sequence: ", data[3] == "1");
        loopSequence = data[3] == "1";
      }
    }
  }

  if (data[0] == "E") {  // EEPROM
    // E:S
    //   ⮡ Save
    // E:L
    //   ⮡ Load

    // E:S:S:0
    //     ⮡ Sequence
    //       ⮡ Slot 1 a 4
    // E:S:C:0
    //     ⮡ Calibracao
    //       ⮡ Joint 1 a 6
    uint8_t target = data[3].toInt() - 1;
    if (data[2] == "S") {    // Sequence
      if (data[1] == "S") {  // Salvar
        Eeprom.saveSequence(target, numSteps, sequence);
      }
      if (data[1] == "L") {  // Load
        Eeprom.loadSequence(target, numSteps, sequence);
      }
    }
    if (data[2] == "C") {
      // uint8_t numPoints = joint[target]->getNumCalibPoints();
      // const uint16_t* calibPoints = joint[target]->getCalibFeedback();

      if (data[1] == "S") {  // Salvar
        // serialPrintln("Salvando calibração: Joint ", data[3]);
        // serialPrintln("Num calib points: ", numPoints);
        // serialPrintln("Calib points: ");
        // for (int c=0; c < numPoints; c++) {
        //   serialPrintln("    Ponto ", c, " -> ", calibPoints[c]);
        // }
        Eeprom.saveCalibration(target, joint[target]);
      }
      if (data[1] == "L") {  // Load
        Eeprom.loadCalibration(target, joint[target]);
      }
    }
  }
}

void executeSequence() {
  if (!executingSequence || currentStep >= numSteps) return;

  Cmd& c = sequence[currentStep];

  bool movementCommand = (c.type == CmdType::MOVE || c.type == CmdType::MOVESMOOTH);
  bool waitCommand = (c.type == CmdType::WAIT);
  bool gripCommand = (c.type == CmdType::GRIP);

  // Envio do passo atual no início da execução
  if (!stepStarted) {
    serialPrintln("#STEP:" + String(currentStep + 1) + "#");
    stepStarted = true;  // flag global ou estática para não repetir durante o mesmo passo

    switch (c.type) {
      case CmdType::MOVE:
      case CmdType::MOVESMOOTH:
        for (int i = 0; i < 5; i++) {
          if (joint[i]->getAngle() != c.move[i]) {
            if (c.type == CmdType::MOVE)
              joint[i]->move(c.move[i], c.speed);
            else
              joint[i]->moveSmooth(c.move[i], c.speed);
          }
        }
        break;

      case CmdType::GRIP:
        if (c.grip) {
          fecharGarra();
        } else {
          abrirGarra();
        }
        // não retorna ainda — deixa a verificação abaixo cuidar da finalização
        break;
    }
  }
  if (c.type == CmdType::WAIT) {
    if (waitStart == 0) waitStart = millis();
    if (millis() - waitStart <= c.wait) {
      return;
    }
    waitStart = 0;
  }

  bool allReached = true;
  if (movementCommand) {
    // Verifica se TODAS as juntas chegaram
    for (int i = 0; i < 5; i++) {
      joint[i]->update();
      if (joint[i]->isMoving()) allReached = false;
      // if (joint[i]->getAngle() != c.move[i])
    }
  } else if (gripCommand) {
    // Verifica se a garra terminou
    if (!gripReached) allReached = false;
  }

  if (allReached) {
    if (lastMoveEnd == 0) lastMoveEnd = millis();
    if (millis() - lastMoveEnd < MOVE_DELAY) return;

    if (currentStep < numSteps -1) {
      nextStep();
    } else {
      serialPrintln("#STEP:END#");
      stepStarted = false;
      lastMoveEnd = 0;
      startBeginning = true;
      if (loopSequence) {
        currentStep = 0;  // Reiniciar para o primeiro movimento
      } else {
        executingSequence = false;  // Stop
      }
    }
  }
}


void nextStep() {
  currentStep++;
  stepStarted = false;
  lastMoveEnd = 0;
}

void fecharGarra() {
  // Comando de fechar
  // serialPrintln("Fechar garra");
  joint[5]->move(26, 10);
  gripReached = false;
  gripSituation = true;
}
void abrirGarra() {
  // Comando de abrir
  // serialPrintln("Abrir garra");
  joint[5]->move(110, 10);
  gripReached = false;
  gripSituation = false;
}


template<typename T, size_t N>
void printArray(const T (&arr)[N]) {
  for (size_t i = 0; i < N; i++) {
    serialPrint(arr[i]);
    if (i < N - 1) serialPrint("; ");
  }
}


int parseAnglesFromHash(const String& field, int out[], int maxOut) {  // String, array, tamanho do array  ||  "#90;0;180;90;90#", angles[5], 5
  int start = field.indexOf('#');
  int end = field.lastIndexOf('#');
  if (start == -1 || end == -1 || end <= start) return 0;

  String inner = field.substring(start + 1, end);  // sem os hashes
  String temp = "";
  int count = 0;

  for (unsigned int i = 0; i < inner.length(); ++i) {
    char c = inner[i];
    if (c == ';') {
      if (temp.length() > 0 && count < maxOut) {
        out[count++] = temp.toInt();
      } else if (temp.length() > 0) {
        // overflow: ignora valores extras
      }
      temp = "";
    } else {
      temp += c;
    }
  }
  // último token
  if (temp.length() > 0 && count < maxOut) {
    out[count++] = temp.toInt();
  }

  return count;
}


template<typename T>
String typeName(const T& var) {
  String s = __PRETTY_FUNCTION__;
  int start = s.indexOf("[with T = ") + 10;
  int stop = s.lastIndexOf(']');
  return s.substring(start, stop);
}


// Função base: imprime um único argumento
template<typename T>
void serialPrintSingle(T arg) {
  Serial.print(arg);
  // #ifdef ESP32
  //   if (SerialBT.hasClient()) SerialBT.print(arg);
  // #endif
}

// Função base com espaço, usada recursivamente
template<typename T, typename... Args>
void serialPrintSingle(T first, Args... rest) {
  serialPrintSingle(first);
  if constexpr (sizeof...(rest) > 0) {
    Serial.print("");
    // #ifdef ESP32
    //     if (SerialBT.hasClient()) SerialBT.print("");
    // #endif
    serialPrintSingle(rest...);
  }
}

// Função principal: imprime sem quebra de linha
template<typename... Args>
void serialPrint(Args... args) {
  serialPrintSingle(args...);
}

// Função principal: imprime com quebra de linha
template<typename... Args>
void serialPrintln(Args... args) {
  serialPrintSingle(args...);
  Serial.println();
  // #ifdef ESP32
  //   if (SerialBT.hasClient()) SerialBT.println();
  // #endif
}
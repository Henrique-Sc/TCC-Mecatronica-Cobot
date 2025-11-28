#pragma once
#include "config.h"
#include <Adafruit_PWMServoDriver.h>

// -------------------------------------------------------------
// Malha base de calibração — usada como referência para gerar
// a malha real conforme os limites físicos de cada servo.
// -------------------------------------------------------------
static const uint8_t BASE_CALIB_ANGLES[BASE_MESH_SIZE] = {
  0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
  100, 110, 120, 130, 140, 150, 160, 170, 180
};


// =============================================================
// Classe Servo — modelo genérico para controle de servos com
// leitura de feedback, calibração e suporte a espelhamento.
// =============================================================
template<typename PWM>
class Servo {
private:
  // ================================
  // --- Atributos internos ---
  // ================================
  PWM* pwm;
  uint8_t joint = 0;
  uint8_t channel = 0;
  int8_t feedbackPin = -1;

  Servo* mirror = nullptr;
  bool isMirror = false;

  uint8_t minPos = 0;
  uint8_t maxPos = 0;

  int16_t minPot = 0;
  int16_t maxPot = 0;
  uint16_t feedback = 0;

  // --- Malha de calibração ---
  uint8_t calibAngles[BASE_MESH_SIZE];
  uint16_t calibFeedback[BASE_MESH_SIZE];
  uint8_t numCalibPoints = 0;
  bool calibrated = false;

  // --- Movimento ---
  uint8_t currentAngle = 0;
  uint8_t targetAngle = 0;
  int8_t initialSpeed = 0;
  int8_t direction = 1;
  uint8_t sleep = 20;

  bool smooth = false;
  bool moving = false;
  uint8_t posAtivacao = 0;

  // --- Controle de tempo ---
  unsigned long lastUpdate = 0;
  unsigned long lastFeedBackUpdate = 0;
  unsigned long feedbackUpdateInterval = 30;

  // --- Logs ---
  bool log_in_move = true;
  bool log = false;


  // =============================================================
  // --- Métodos auxiliares privados ---
  // =============================================================

  bool isFeedbackPinValid() { return feedbackPin != -1; }

  void setIsMirror() { isMirror = true; }

  uint16_t grauParaPulso(uint8_t grau) {
    return map(grau, 0, 180, SERVO_MIN_PWM, SERVO_MAX_PWM);
  }

  uint8_t calcularSleep(uint8_t speed) {
    return constrain(map(constrain(speed, 1, 10), 1, 10, 20, 0), 3, 20);
  }


public:
  // =============================================================
  // --- Construtor e configuração básica ---
  // =============================================================

  Servo(PWM& pwmDriver, uint8_t channel, uint8_t joint, int8_t feedbackPin,
        uint8_t minPos, uint8_t maxPos, uint8_t currentAngle) {
    this->pwm = &pwmDriver;
    this->channel = channel;
    this->joint = joint;
    this->minPos = minPos;
    this->maxPos = maxPos;
    this->currentAngle = currentAngle;
    this->targetAngle = currentAngle;
    if (feedbackPin != -1) this->feedbackPin = feedbackPin;
  }

  void attach() {
    this->write(currentAngle); // já atinge servo e espelho (se existir)
  }

  void detach() {
    this->pwm->setPWM(channel, 0, 0);
    if (mirror) mirror->detach();
  }


  // =============================================================
  // --- Escrita e leitura ---
  // =============================================================

  void write(int angle, bool fromMaster = false) {
    if (isMirror && !fromMaster) return;

    currentAngle = constrain(angle, minPos, maxPos);
    pwm->setPWM(channel, 0, grauParaPulso(currentAngle));

    // Espelhar o movimento, invertendo ângulo
    if (mirror) mirror->write(map(angle, 0, 180, 180, 0), true);

    if (!isMirror) {
      serialPrintln("#J", this->joint, ":", currentAngle, "#");  // Ex: #J1:90#
      if (log_in_move && !log) display();
    }
  }

  void setPwm(uint16_t pwmValue, bool fromMaster = false) {
    pwm->setPWM(channel, 0, pwmValue);
    if (log_in_move && !log) display();
  }

  int getAngle() { return currentAngle; }
  uint8_t getTargetAngle() { return targetAngle; }

  void setAngle(uint8_t angle) {
    currentAngle = constrain(angle, minPos, maxPos);
  }

  // =============================================================
  // --- Feedback (sensor interno do servo) ---
  // =============================================================

  int16_t getFeedback() {
    if (!isFeedbackPinValid()) return -1;

    // Caso sem calibração
    if (!calibrated || numCalibPoints < 2) {
      return constrain(map(feedback, minPot, maxPot, minPos, maxPos), minPos, maxPos);
    }

    int val = feedback;
    bool adcIncreasing = (calibFeedback[numCalibPoints - 1] >= calibFeedback[0]);

    int adcMin = min((int)calibFeedback[0], (int)calibFeedback[numCalibPoints - 1]);
    int adcMax = max((int)calibFeedback[0], (int)calibFeedback[numCalibPoints - 1]);

    // Fora do range
    if (val <= adcMin || val >= adcMax) {
      int idx = (val <= adcMin)
                  ? (calibFeedback[0] <= calibFeedback[numCalibPoints - 1] ? 0 : numCalibPoints - 1)
                  : (calibFeedback[0] > calibFeedback[numCalibPoints - 1] ? 0 : numCalibPoints - 1);
      return constrain((int16_t)round(calibAngles[idx]), minPos, maxPos);
    }

    // Interpolação linear entre pontos
    for (int i = 0; i < numCalibPoints - 1; i++) {
      int f1 = calibFeedback[i], f2 = calibFeedback[i + 1];
      if (f1 == f2) continue;

      bool inSegment = adcIncreasing ? (val >= f1 && val <= f2) : (val <= f1 && val >= f2);
      if (!inSegment) continue;

      float frac = (float)(val - f1) / (float)(f2 - f1);
      float angle = (float)calibAngles[i] + frac * ((float)calibAngles[i + 1] - (float)calibAngles[i]);
      return constrain((int16_t)round(angle), minPos, maxPos);
    }

    // fallback (caso raro)
    return constrain(currentAngle, minPos, maxPos);
  }

  void feedbackFilter() {
    feedback = (ALPHA_MEDIA_MOVEL * analogRead(feedbackPin) +
               (1 - ALPHA_MEDIA_MOVEL) * feedback);
  }


  // =============================================================
  // --- Movimento ---
  // =============================================================

  void step(int8_t delta) {
    currentAngle = constrain(currentAngle + delta, minPos, maxPos);
    write(currentAngle);
  }

  void move(uint8_t target, uint8_t speed) {
    target = constrain(target, minPos, maxPos);
    targetAngle = target;
    initialSpeed = speed;
    sleep = calcularSleep(speed);
    moving = true;
    smooth = false;
    direction = (target - currentAngle > 0) ? 1 : (target - currentAngle < 0 ? -1 : 0);
  }

  void moveSmooth(uint8_t target, uint8_t speed) {
    move(target, speed);
    smooth = true;
    posAtivacao = target - (abs(target - currentAngle) * 0.5 * direction);
  }

  void update() {
    // Atualiza feedback periodicamente
    if (millis() - lastFeedBackUpdate >= feedbackUpdateInterval) {
      lastFeedBackUpdate = millis();
      feedbackFilter();
    }

    // Controle de movimento
    if (moving && (millis() - lastUpdate >= sleep)) {
      if (smooth) {
        if ((direction > 0 && currentAngle >= posAtivacao) ||
            (direction < 0 && currentAngle <= posAtivacao)) {
          uint16_t dif = abs(targetAngle - currentAngle);
          sleep = calcularSleep(map(dif, 1, abs(targetAngle - posAtivacao),
                                    constrain(initialSpeed - 6, 1, 10), initialSpeed - 1));
        }
      }

      step(direction);
      lastUpdate = millis();

      if (currentAngle == targetAngle) moving = false;
    }

    if (log) display();
  }

  bool isMoving() { return moving; }


  // =============================================================
  // --- Calibração ---
  // =============================================================

  void generateCalibMesh() {
    numCalibPoints = 0;
    for (int i = 0; i < BASE_MESH_SIZE; i++) {
      uint8_t ang = BASE_CALIB_ANGLES[i];
      if (ang < minPos || ang > maxPos) continue;
      calibAngles[numCalibPoints++] = ang;
    }
    if (numCalibPoints == 0 || calibAngles[0] != minPos) calibAngles[0] = minPos;
    if (calibAngles[numCalibPoints - 1] != maxPos) calibAngles[numCalibPoints++] = maxPos;

    serialPrint("Servo ", joint, " | Malha: ");
    for (int i = 0; i < numCalibPoints; i++) serialPrint(calibAngles[i], " ");
    serialPrintln("");
  }

  void calibrateFeedbackMultiPoint() {
    if (!isFeedbackPinValid()) return;

    bool log_state = log;
    bool log_in_move_state = log_in_move;
    log = false;
    log_in_move = false;

    serialPrintln("Iniciando calibracao multiponto com filtro exponencial...");

    for (int i = 0; i < numCalibPoints; i++) {
      uint8_t ang = calibAngles[i];
      move(ang, 1);
      while (isMoving()) update();
      delay(300);

      for (int j = 0; j < 50; j++) { feedbackFilter(); delay(10); }
      for (int j = 0; j < 100; j++) { feedbackFilter(); delay(20); }

      calibFeedback[i] = feedback;
      serialPrintln("Ponto ", i + 1, ": ", ang, "° = ", calibFeedback[i]);
    }

    log = log_state;
    log_in_move = log_in_move_state;
    calibrated = true;

    serialPrintln("Calibracao multiponto concluida!");
  }

  // uint8_t getNumCalibPoints() const { return numCalibPoints; }

  // void getCalibFeedback(uint16_t* dest) const {
  //   for (uint8_t i = 0; i < BASE_MESH_SIZE; i++) dest[i] = calibFeedback[i];
  // }

  // void setCalibFeedback(uint8_t numPoints, const uint16_t* src) {
  //   numCalibPoints = numPoints;
  //   for (uint8_t i = 0; i < BASE_MESH_SIZE; i++) calibFeedback[i] = src[i];
  //   calibrated = true;
  // }

  // uint16_t calibFeedback[BASE_MESH_SIZE];
  // uint8_t numCalibPoints = 0;

  uint8_t getNumCalibPoints() {
    return this->numCalibPoints;
  }
  
  void setNumCalibPoints(uint8_t numPoints) {
    this->numCalibPoints = numPoints;
  }

  const uint16_t* getCalibFeedback() const {
    return this->calibFeedback;
  }

  void setCalibFeedback(const uint16_t* feedbackPtr) {
    for (uint8_t i = 0; i < BASE_MESH_SIZE; i++) {
      calibFeedback[i] = feedbackPtr[i];
    }
    this->calibrated = true;
  }


  // =============================================================
  // --- Espelhamento ---
  // =============================================================

  void setMirror(Servo* mirrorServo) {
    mirrorServo->setIsMirror();
    mirror = mirrorServo;
  }

  bool getIsMirror() const { return isMirror; }


  // =============================================================
  // --- Logs e depuração ---
  // =============================================================

  void display() {
    serialPrint("J", joint);
    if (mirror) serialPrint(".1");
    if (isMirror) serialPrint(".2");
    serialPrint(" | Angle: ", currentAngle, "°");
    if (isFeedbackPinValid()) {
      serialPrint(" | Pos analog: ", feedback, " | Feedback: ", getFeedback(), "°");
    }
    serialPrintln("");
  }

  void setLog(bool value) { log = value; }
  void setLogInMove(bool value) { log_in_move = value; }
};

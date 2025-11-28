#include <stddef.h>
#pragma once


// ========================================
//       --- DEFINIÇÃO DE CONSTANTES ---
// ========================================

#define ESP32

#define SERVO_MIN_PWM 162
#define SERVO_MAX_PWM 632

#define ALPHA_MEDIA_MOVEL 0.1

#define EEPROM_SIZE 4096
#define NUM_JOINTS 6
#define MAX_STEPS 30
#define MAX_SEQUENCES 4
#define BASE_MESH_SIZE 19



// ======= USANDO Saídas digitais do Arduino =======
// #define SERVO_0_CHANNEL 3
// #define SERVO_1_CHANNEL 4
// #define SERVO_2_CHANNEL 5
// #define SERVO_3_CHANNEL 6
// #define SERVO_4_CHANNEL 7
// #define SERVO_5_CHANNEL 8
// #define SERVO_6_CHANNEL 9
// #define SERVO_7_CHANNEL 10

// ======= USANDO PCA_9685 (Adafruit_PWMServoDriver) =======
#define SERVO_0_CHANNEL 0
#define SERVO_1_CHANNEL 1
#define SERVO_2_CHANNEL 2
#define SERVO_3_CHANNEL 3
#define SERVO_4_CHANNEL 4
#define SERVO_5_CHANNEL 5
#define SERVO_6_CHANNEL 6
#define SERVO_7_CHANNEL 7


// ======= VERSÃO ESP32 =======
#define FEEDBACK_0_PIN 32
#define FEEDBACK_1_PIN 33
#define FEEDBACK_2_PIN -1
#define FEEDBACK_3_PIN 35
#define FEEDBACK_4_PIN 34
#define FEEDBACK_5_PIN -1
#define FEEDBACK_6_PIN 36
#define FEEDBACK_7_PIN 39

#define BTN_1 23 
#define BTN_2 19
#define BTN_3 18



// ======= TEMPLATES =======
// template<typename T>
// void sendSerialData(T msg, bool breakline);

template<typename T, size_t N>
void printArray(const T (&arr)[N]);

template<typename... Args>
void serialPrint(Args... args);

template<typename... Args>
void serialPrintln(Args... args);
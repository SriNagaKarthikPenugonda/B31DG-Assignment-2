#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "B31DGMonitor.h"

#define SIGNAL1_PIN 25
#define SIGNAL2_PIN 26
#define FREQUENCY1_PIN 34
#define FREQUENCY2_PIN 35
#define LED_SUM_PIN 2
#define LED_BUTTON_PIN 18
#define BUTTON_PIN 33

B31DGCyclicExecutiveMonitor monitor;

volatile int frequency1 = 0;
volatile int frequency2 = 0;
int f1_count = 0, f2_count = 0;
int lastF1State = LOW, lastF2State = LOW;
unsigned long lastF1Check = 0, lastF2Check = 0;

bool ledState = false;
bool buttonModeToggle = false;
unsigned long lastButtonPress = 0;

SemaphoreHandle_t freqMutex;

void task1_signal1(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    monitor.jobStarted(1);
    digitalWrite(SIGNAL1_PIN, HIGH); delayMicroseconds(250);
    digitalWrite(SIGNAL1_PIN, LOW); delayMicroseconds(50);
    digitalWrite(SIGNAL1_PIN, HIGH); delayMicroseconds(300);
    digitalWrite(SIGNAL1_PIN, LOW);
    monitor.jobEnded(1);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(4));
  }
}

void task2_signal2(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    monitor.jobStarted(2);
    digitalWrite(SIGNAL2_PIN, HIGH); delayMicroseconds(100);
    digitalWrite(SIGNAL2_PIN, LOW); delayMicroseconds(50);
    digitalWrite(SIGNAL2_PIN, HIGH); delayMicroseconds(200);
    digitalWrite(SIGNAL2_PIN, LOW);
    monitor.jobEnded(2);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(3));
  }
}

void task3_measure_freq1(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    monitor.jobStarted(3);
    int state = digitalRead(FREQUENCY1_PIN);
    if (lastF1State == LOW && state == HIGH) {
      f1_count++;
    }
    lastF1State = state;

    if (millis() - lastF1Check >= 10) {
      if (xSemaphoreTake(freqMutex, portMAX_DELAY)) {
        frequency1 = f1_count * 755;
        f1_count = 0;
        xSemaphoreGive(freqMutex);
      }
      lastF1Check = millis();
    }
    monitor.jobEnded(3);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1));
  }
}

void task4_measure_freq2(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    monitor.jobStarted(4);
    int state = digitalRead(FREQUENCY2_PIN);
    if (lastF2State == LOW && state == HIGH) {
      f2_count++;
    }
    lastF2State = state;

    if (millis() - lastF2Check >= 10) {
      if (xSemaphoreTake(freqMutex, portMAX_DELAY)) {
        frequency2 = f2_count * 750;
        f2_count = 0;
        xSemaphoreGive(freqMutex);
      }
      lastF2Check = millis();
    }
    monitor.jobEnded(4);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1));
  }
}

void task5_monitor(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    monitor.jobStarted(5);
    monitor.doWork();
    monitor.jobEnded(5);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
  }
}

void task6_sum_check(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    int f1, f2;
    if (xSemaphoreTake(freqMutex, portMAX_DELAY)) {
      f1 = frequency1;
      f2 = frequency2;
      xSemaphoreGive(freqMutex);
    }

    digitalWrite(LED_SUM_PIN, (f1 + f2) > 1500 ? HIGH : LOW);
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2));
  }
}

void task7_button(void *pvParameters) {
  TickType_t lastWake = xTaskGetTickCount();
  while (true) {
    bool buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && millis() - lastButtonPress > 200) {
      lastButtonPress = millis();
      buttonModeToggle = !buttonModeToggle;

      if (!buttonModeToggle) {
        digitalWrite(LED_BUTTON_PIN, HIGH);
        ledState = true;
      } else {
        ledState = !ledState;
        digitalWrite(LED_BUTTON_PIN, ledState);
      }

      monitor.doWork();
    }

    if (!buttonModeToggle) {
      digitalWrite(LED_BUTTON_PIN, HIGH);
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(SIGNAL1_PIN, OUTPUT);
  pinMode(SIGNAL2_PIN, OUTPUT);
  pinMode(FREQUENCY1_PIN, INPUT);
  pinMode(FREQUENCY2_PIN, INPUT);
  pinMode(LED_SUM_PIN, OUTPUT);
  pinMode(LED_BUTTON_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_BUTTON_PIN, HIGH);
  ledState = true;

  monitor.startMonitoring();

  freqMutex = xSemaphoreCreateMutex();
  if (freqMutex == NULL) {
    Serial.println("Failed to create mutex!");
    while (1);
  }

  xTaskCreatePinnedToCore(task1_signal1, "Signal1", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(task2_signal2, "Signal2", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(task5_monitor, "Monitor", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(task3_measure_freq1, "Freq1", 4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(task4_measure_freq2, "Freq2", 4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(task6_sum_check, "SumCheck", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(task7_button, "Button", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(1);
}
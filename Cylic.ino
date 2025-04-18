#include <Arduino.h>
#include "B31DGMonitor.h"

#define SIGNAL1_PIN 25
#define SIGNAL2_PIN 26
#define FREQUENCY1_PIN 34
#define FREQUENCY2_PIN 35
#define LED_SUM_PIN 2
#define LED_BUTTON_PIN 18
#define BUTTON_PIN 33

B31DGCyclicExecutiveMonitor monitor;

bool buttonModeToggle = false;
int frequency1 = 0;
int frequency2 = 0;

unsigned long lastF1Check = 0, lastF2Check = 0;
int f1_count = 0, f2_count = 0;
int lastF1State = LOW, lastF2State = LOW;
unsigned long lastButtonPress = 0;
bool ledState = false;

unsigned long currentTime = 0;
unsigned long lastTask1 = 0, lastTask2 = 0, lastTask3 = 0, lastTask4 = 0, lastTask5 = 0, lastTask6 = 0, lastTask7 = 0;

void task1_digital_signal1() {
    monitor.jobStarted(1);
    digitalWrite(SIGNAL1_PIN, HIGH); delayMicroseconds(250);
    digitalWrite(SIGNAL1_PIN, LOW);  delayMicroseconds(50);
    digitalWrite(SIGNAL1_PIN, HIGH); delayMicroseconds(300);
    digitalWrite(SIGNAL1_PIN, LOW);
    monitor.jobEnded(1);
}

void task2_digital_signal2() {
    monitor.jobStarted(2);
    digitalWrite(SIGNAL2_PIN, HIGH); delayMicroseconds(100);
    digitalWrite(SIGNAL2_PIN, LOW);  delayMicroseconds(50);
    digitalWrite(SIGNAL2_PIN, HIGH); delayMicroseconds(200);
    digitalWrite(SIGNAL2_PIN, LOW);
    monitor.jobEnded(2);
}

void task3_measure_frequency1() {
    monitor.jobStarted(3);
    if (currentTime - lastF1Check >= 10) {
        frequency1 = f1_count * 760;
        f1_count = 0;
        lastF1Check = currentTime;
    }
    int state = digitalRead(FREQUENCY1_PIN);
    if (lastF1State == LOW && state == HIGH) {
        f1_count++;
    }
    lastF1State = state;
    monitor.jobEnded(3);
}

void task4_measure_frequency2() {
    monitor.jobStarted(4);
    if (currentTime - lastF2Check >= 10) {
        frequency2 = f2_count * 750;
        f2_count = 0;
        lastF2Check = currentTime;
    }
    int state = digitalRead(FREQUENCY2_PIN);
    if (lastF2State == LOW && state == HIGH) {
        f2_count++;
    }
    lastF2State = state;
    monitor.jobEnded(4);
}

void task5_monitor_work() {
    monitor.jobStarted(5);
    delayMicroseconds(100);
    monitor.doWork();
    monitor.jobEnded(5);
}

void task6_check_sum() {
    if ((frequency1 + frequency2) > 1500) {
        digitalWrite(LED_SUM_PIN, HIGH);
    } else {
        digitalWrite(LED_SUM_PIN, LOW);
    }
}

void task7_check_button() {
    bool state = digitalRead(BUTTON_PIN);
    if (state == LOW && currentTime - lastButtonPress > 200) {
        lastButtonPress = currentTime;
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
        ledState = true;
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
}

void loop() {
    currentTime = millis();

    if (currentTime - lastTask1 >= 4) {
        lastTask1 = currentTime;
        task1_digital_signal1();
    }
    if (currentTime - lastTask2 >= 3) {
        lastTask2 = currentTime;
        task2_digital_signal2();
    }
    if (currentTime - lastTask3 >= 10) {
        lastTask3 = currentTime;
        task3_measure_frequency1();
    }
    if (currentTime - lastTask4 >= 10) {
        lastTask4 = currentTime;
        task4_measure_frequency2();
    }
    if (currentTime - lastTask5 >= 5) {
        lastTask5 = currentTime;
        task5_monitor_work();
    }
    if (currentTime - lastTask6 >= 1) {
        lastTask6 = currentTime;
        task6_check_sum();
    }
    if (currentTime - lastTask7 >= 10) {
        lastTask7 = currentTime;
        task7_check_button();
    }

    delay(1);
}

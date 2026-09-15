#include <Arduino.h>

struct Config {
    static constexpr uint8_t LED_PIN = 21;
    static constexpr uint8_t BUTTON_PIN = 18;
    static constexpr uint32_t BLINK_INTERVAL = 500;
    static constexpr uint32_t DEBOUNCE_DELAY = 50; // мс для антидребезгу
    static constexpr uint16_t MEASURE_ITERATIONS = 1000;
};

enum class LedState { Off, On };

// Режими роботи
enum class WorkMode {
    Blinking,
    AlwaysOn,
    AlwaysOff
};

class Led {
private:
    uint8_t pin;
public:
    constexpr Led(uint8_t p) : pin(p) {}
    void init() const {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    void set(LedState state) const {
        digitalWrite(pin, state == LedState::On ? HIGH : LOW);
    }
};

Led myLed(Config::LED_PIN);
LedState currentLedState = LedState::Off;
WorkMode currentMode = WorkMode::Blinking;

uint32_t lastBlinkTime = 0;
uint32_t lastDebounceTime = 0;

uint32_t iterationCount = 0;
uint32_t totalMicros = 0;

// Прапорець переривання (обов'язково volatile)
volatile bool buttonPressedISR = false;

// Функція переривання (мінімальна, в пам'яті IRAM для ESP32)
void IRAM_ATTR handleButtonInterrupt() {
    buttonPressedISR = true;
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("System started!");
    myLed.init();
    
    // Налаштування кнопки з внутрішньою підтяжкою до живлення
    pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
    
    // Переривання по спаду (FALLING), бо підтяжка до HIGH, натискання замикає на LOW (GND)
    attachInterrupt(digitalPinToInterrupt(Config::BUTTON_PIN), handleButtonInterrupt, FALLING);
}

void loop() {
    uint32_t startMicros = micros();
    uint32_t currentMillis = millis();

    // 1. Обробка кнопки (Антидребезг і логіка у superloop)
    if (buttonPressedISR) {
        buttonPressedISR = false; // Скидаємо прапорець

        // Перевіряємо антидребезг
        if (currentMillis - lastDebounceTime > Config::DEBOUNCE_DELAY) {
            lastDebounceTime = currentMillis;

            // Перемикаємо режими: Blinking -> AlwaysOn -> AlwaysOff -> Blinking...
            if (currentMode == WorkMode::Blinking) {
                currentMode = WorkMode::AlwaysOn;
            } else if (currentMode == WorkMode::AlwaysOn) {
                currentMode = WorkMode::AlwaysOff;
            } else {
                currentMode = WorkMode::Blinking;
            }
        }
    }

    // 2. Логіка роботи LED відповідно до поточного режиму
    switch (currentMode) {
        case WorkMode::Blinking:
            if (currentMillis - lastBlinkTime >= Config::BLINK_INTERVAL) {
                lastBlinkTime = currentMillis;
                currentLedState = (currentLedState == LedState::Off) ? LedState::On : LedState::Off;
                myLed.set(currentLedState);
            }
            break;
        case WorkMode::AlwaysOn:
            myLed.set(LedState::On);
            break;
        case WorkMode::AlwaysOff:
            myLed.set(LedState::Off);
            break;
    }

    // 3. Вимірювання часу superloop
    totalMicros += (micros() - startMicros);
    iterationCount++;

    if (iterationCount >= Config::MEASURE_ITERATIONS) {
        float avgTime = (float)totalMicros / Config::MEASURE_ITERATIONS;
        Serial.print("Avg loop time (us): ");
        Serial.println(avgTime);
        iterationCount = 0;
        totalMicros = 0;
    }
}
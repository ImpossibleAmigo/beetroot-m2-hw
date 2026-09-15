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
uint32_t startMeasureMicros = 0; // Для правильного вимірювання пачки ітерацій

// Прапорець переривання (обов'язково volatile)
volatile bool buttonPressedISR = false;

// Функція переривання
void IRAM_ATTR handleButtonInterrupt() {
    buttonPressedISR = true;
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Даємо час порту для ініціалізації
    Serial.println("System started! Setup is done.");

    myLed.init();
    
    // Налаштування кнопки
    pinMode(Config::BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Config::BUTTON_PIN), handleButtonInterrupt, FALLING);
}

void loop() {
    // Фіксуємо час на початку першої ітерації з пачки
    if (iterationCount == 0) {
        startMeasureMicros = micros();
    }

    uint32_t currentMillis = millis();

    // 1. Обробка кнопки (Антидребезг)
    if (buttonPressedISR) {
        buttonPressedISR = false;
        
        if (currentMillis - lastDebounceTime > Config::DEBOUNCE_DELAY) {
            lastDebounceTime = currentMillis;
            
            if (currentMode == WorkMode::Blinking) {
                currentMode = WorkMode::AlwaysOn;
            } else if (currentMode == WorkMode::AlwaysOn) {
                currentMode = WorkMode::AlwaysOff;
            } else {
                currentMode = WorkMode::Blinking;
            }
        }
    }

    // 2. Логіка роботи LED
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
    iterationCount++;

    if (iterationCount >= Config::MEASURE_ITERATIONS) {
        uint32_t totalTimeFor1000 = micros() - startMeasureMicros; 
        float avgTime = (float)totalTimeFor1000 / Config::MEASURE_ITERATIONS;
        
        Serial.print("Avg loop time (us): ");
        Serial.println(avgTime, 4); // Вивід з 4 знаками після коми
        
        iterationCount = 0; 
    }
}
#include <Arduino.h>

struct Config {
    static constexpr uint8_t LED_PIN = 21;
    static constexpr uint32_t BLINK_INTERVAL = 500;
    static constexpr uint16_t MEASURE_ITERATIONS = 1000;
};

enum class LedState { Off, On };

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
uint32_t lastBlinkTime = 0;

// Змінні для вимірювання часу
uint32_t iterationCount = 0;
uint32_t totalMicros = 0;

void setup() {
    Serial.begin(115200);
    myLed.init();
}

void loop() {
    uint32_t startMicros = micros(); // Початок ітерації
    uint32_t currentMillis = millis();

    if (currentMillis - lastBlinkTime >= Config::BLINK_INTERVAL) {
        lastBlinkTime = currentMillis;
        currentLedState = (currentLedState == LedState::Off) ? LedState::On : LedState::Off;
        myLed.set(currentLedState);
    }

    // Вимірювання та вивід
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
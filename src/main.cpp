#include <Arduino.h>

// Конфігурація системи (замість магічних чисел)
struct Config {
    static constexpr uint8_t LED_PIN = 21;
    static constexpr uint32_t BLINK_INTERVAL = 500; // мілісекунди
};

// Стан світлодіода
enum class LedState {
    Off,
    On
};

// Клас для роботи з LED
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

void setup() {
    myLed.init();
}

void loop() {
    uint32_t currentMillis = millis();

    // Неблокуюче блимання
    if (currentMillis - lastBlinkTime >= Config::BLINK_INTERVAL) {
        lastBlinkTime = currentMillis;
        currentLedState = (currentLedState == LedState::Off) ? LedState::On : LedState::Off;
        myLed.set(currentLedState);
    }
}
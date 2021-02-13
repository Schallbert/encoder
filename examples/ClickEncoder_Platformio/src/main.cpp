#include <Arduino.h>

#include "../../../ClickEncoder.cpp"

// Port Pin IDs for Arduino
constexpr uint8_t PIN_ENCA = 4;
constexpr uint8_t PIN_ENCB = 5;
constexpr uint8_t PIN_BTN = 3;

constexpr uint16_t SERIAL_BAUDRATE = 9600;
constexpr uint8_t PRINT_BASE = 10;

// interval in which encoder values are fetched and printed in this demo
constexpr uint16_t PRINTINTERVAL_MS = 100;

ClickEncoder *testEncoder{nullptr};

// --- forward-declared function prototypes:
// Prints out button state
void printClickEncoderButtonState();
// Prints turn information (turn status, direction, Acceleration value)
void printClickEncoderValue();
// Prints accumulated turn information
void printClickEncoderCount();

void setup()
{
    // Use the serial connection to print out encoder's behavior
    Serial.begin(SERIAL_BAUDRATE);
    // Setup and configure "full-blown" ClickEncoder
    testEncoder = new ClickEncoder(PIN_ENCA, PIN_ENCB, PIN_BTN, 4, false);
    testEncoder->setAccelerationEnabled(true);
    testEncoder->setDoubleClickEnabled(true);
    testEncoder->setLongPressRepeatEnabled(true);

    Serial.println("Hi! This is the ClickEncoder Test Program.");
    Serial.println("When connected correctly: turn right should increase the value.");
}

void loop()
{
    static volatile uint16_t readIntervalCount{0};
    // In real applications, please use an interrupt-driven timer (e.g. TimerOne library)
    _delay_ms(1);
    // This is the Encoder's worker routine. It will physically read the hardware
    // and all most of the logic happens here. Recommended interval for this method is 1ms.
    testEncoder->service();

    // Reads Encoder's status/values and prints to serial for demonstration.
    if (readIntervalCount >= PRINTINTERVAL_MS)
    {
        readIntervalCount = 0;

        printClickEncoderButtonState();
        printClickEncoderValue();
        //printClickEncoderCount();
    }
    ++readIntervalCount;
}

void printClickEncoderButtonState()
{
    switch (testEncoder->getButton())
    {
    case Button::Clicked:
        Serial.println("Button clicked");
        break;
    case Button::DoubleClicked:
        Serial.println("Button doubleClicked");
        break;
    case Button::Held:
        Serial.println("Button Held");
        break;
    case Button::LongPressRepeat:
        Serial.println("Button longPressRepeat");
        break;
    case Button::Released:
        Serial.println("Button released");
        break;
    default:
        // no output for "Open" to not spam the terminal.
        break;
    }
}

void printClickEncoderValue()
{
    int16_t value = testEncoder->getIncrement();
    if (value != 0)
    {
        Serial.print("Encoder value: ");
        Serial.println(value, PRINT_BASE);
    }
}

void printClickEncoderCount()
{
    static int16_t lastValue{0};
    int16_t value = testEncoder->getAccumulate();
    if (value != lastValue)
    {
        Serial.print("Encoder count: ");
        Serial.println(value, PRINT_BASE);
    }
    lastValue = value;
}
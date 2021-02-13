// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Held, LongPressRepeat
//
// Refactored, logic upgraded and feature added (LongPressRepeat) by Schallbert 2021
// ----------------------------------------------------------------------------

#ifndef CLICKENCODER_H
#define CLICKENCODER_H

#include <Arduino.h>

// ----------------------------------------------------------------------------
// Acceleration configuration (for 1ms calls to ::service())
//
constexpr uint8_t ENC_ACCEL_START = 64; // Start increasing count > (1/tick) below tick iterval of x ms.
constexpr uint8_t ENC_ACCEL_SLOPE = 16; // below ACCEL_START, velocity progression of 1/x ticks
// Example: increment every 50ms, without acceleration it would count to 20 within 1sec.
// With acceleration START@64 and SLOPE@16 it will count to 80 within 1 sec.

// Button configuration (values for 1ms timer service calls)
//
constexpr uint8_t ENC_BUTTONINTERVAL = 20;            // check button every x ms, also debouce time
constexpr uint16_t ENC_DOUBLECLICKTIME = 400;         // second click within x ms
constexpr uint16_t ENC_LONGPRESSREPEATINTERVAL = 200; // reports repeating-held every x ms
constexpr uint16_t ENC_HOLDTIME = 1200;               // report held button after x ms
// ----------------------------------------------------------------------------

class Encoder
{
public:
    explicit Encoder(uint8_t A, uint8_t B, uint8_t stepsPerNotch = 4, bool active = LOW);
    ~Encoder() = default;
    Encoder(const Encoder &cpyEncoder) = delete;
    Encoder &operator=(const Encoder &srcEncoder) = delete;

    void service();
    int16_t getIncrement();
    int16_t getAccumulate();
    void setAccelerationEnabled(const bool a) { accelerationEnabled = a; };

private:
    uint8_t getBitCode();
    void handleEncoder();
    int8_t handleValues(int8_t moved);

    const uint8_t pinA;
    const uint8_t pinB;
    const uint8_t stepsPerNotch;
    const bool pinActiveState;

    bool accelerationEnabled{false};
    volatile int8_t lastEncoderRead{0};
    volatile int16_t encoderAccumulate{0};
    volatile int16_t lastEncoderAccumulate{0};
    volatile uint8_t lastMovedCount{ENC_ACCEL_START};
};

class Button
{
public:
    enum eButtonStates
    {
        Open = 0,
        Closed,
        Held,
        LongPressRepeat,
        Released,
        Clicked,
        DoubleClicked
    };

    explicit Button(uint8_t BTN, bool active = LOW);
    ~Button() = default;
    Button(const Button &cpyButton) = delete;
    Button &operator=(const Button &srcButton) = delete;

    void service();
    eButtonStates getButton();
    void setDoubleClickEnabled(const bool b) { doubleClickEnabled = b; };
    void setLongPressRepeatEnabled(const bool b) { longPressRepeatEnabled = b; };

private:
    void handleButton();
    void handleButtonPressed();
    void handleButtonReleased();

    const uint8_t pinBTN;
    const bool pinActiveState;

    bool doubleClickEnabled{false};
    bool longPressRepeatEnabled{false};
    volatile eButtonStates buttonState{Open};
    uint8_t doubleClickTicks{0};
    uint16_t keyDownTicks{0};
    uint16_t lastButtonCheckCount{0};
};

class ClickEncoder
{
public:
    explicit ClickEncoder(uint8_t A, uint8_t B, uint8_t BTN = -1,
                 uint8_t stepsPerNotch = 4, bool active = LOW);
    ~ClickEncoder();
    ClickEncoder(const ClickEncoder &cpyEncoder) = delete;
    ClickEncoder &operator=(const ClickEncoder &srcEncoder) = delete;

    void service();
    int16_t getIncrement() { return enc->getIncrement(); };
    int16_t getAccumulate() { return enc->getAccumulate(); };
    Button::eButtonStates getButton() {  return btn->getButton(); };
    void setAccelerationEnabled(const bool b) { enc->setAccelerationEnabled(b); };
    void setDoubleClickEnabled(const bool b) { btn->setDoubleClickEnabled(b); };
    void setLongPressRepeatEnabled(const bool b) { btn->setLongPressRepeatEnabled(b); };

private:
    Encoder* enc{nullptr};
    Button* btn{nullptr};
};
#endif // CLICKENCODER_H
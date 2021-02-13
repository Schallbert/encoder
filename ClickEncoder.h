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

class ClickEncoder
{
public:
    enum eButton
    {
        Open = 0,
        Closed,
        Pressed, // Deprecated - Not used in implementation!

        Held,
        LongPressRepeat,
        Released,

        Clicked,
        DoubleClicked
    };

public:
    ClickEncoder(uint8_t A, uint8_t B, uint8_t BTN = -1,
                 uint8_t stepsPerNotch = 4, bool active = LOW);
    ~ClickEncoder() = default;
    ClickEncoder(const ClickEncoder &cpyEncoder) = delete;
    ClickEncoder &operator=(const ClickEncoder &srcEncoder) = delete;


    void service();
    int16_t getIncrement();
    int16_t getAccumulate();

#ifndef WITHOUT_BUTTON
public:
    eButton getButton();
    void setDoubleClickEnabled(const bool b) { doubleClickEnabled = b; };
    void setLongPressRepeatEnabled(const bool b) { longPressRepeatEnabled = b; };

private:
    uint8_t getBitCode();
    void handleEncoder();
    int8_t handleValues(int8_t moved);
    void handleButton();
    void handleButtonPressed();
    void handleButtonReleased();

#endif

public:
    void setAccelerationEnabled(const bool a) { accelerationEnabled = a; };

private:
    const uint8_t pinA;
    const uint8_t pinB;
    const uint8_t pinBTN;
    const uint8_t stepsPerNotch;
    const bool pinsActive;
#ifndef WITHOUT_BUTTON
    bool doubleClickEnabled;
    bool longPressRepeatEnabled;
#endif
    bool accelerationEnabled;
    volatile int8_t lastEncoderRead;
    volatile int16_t encoderAccumulate;
    volatile int16_t lastEncoderAccumulate;
    volatile uint8_t lastMoved;
#if ENC_DECODER != ENC_NORMAL
    static const int8_t table[16];
#endif
#ifndef WITHOUT_BUTTON
    volatile eButton button;
    uint16_t keyDownTicks{0};
    uint8_t doubleClickTicks{0};
    unsigned long lastButtonCheck{0};
#endif
};

// ----------------------------------------------------------------------------

#endif // CLICKENCODER_H

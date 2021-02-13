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
// eButton configuration (values for 1ms timer service calls)
//
constexpr uint8_t ENC_BUTTONINTERVAL = 20;            // check button every x ms, also debouce time
constexpr uint16_t ENC_DOUBLECLICKTIME = 400;         // second click within x ms
constexpr uint16_t ENC_LONGPRESSREPEATINTERVAL = 200; // reports repeating-held every x ms
constexpr uint16_t ENC_HOLDTIME = 1200;               // report held button after x ms

// ----------------------------------------------------------------------------
// Acceleration configuration (for 1ms calls to ::service())
//
constexpr uint8_t ENC_ACCEL_START = 64; // Start increasing count > (1/tick) below tick iterval of x ms.
constexpr uint8_t ENC_ACCEL_SLOPE = 16; // below ACCEL_START, velocity progression of 1/x ticks
// Example: increment every 50ms, without acceleration it would count to 20 within 1sec.
// With acceleration START@64 and SLOPE@16 it will count to 80 within 1 sec.

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
    void setAccelerationEnabled(const bool a) { accelerationEnabled = a; };

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
    volatile int8_t lastEncoderRead{0};
    volatile int16_t encoderAccumulate{0};
    volatile int16_t lastEncoderAccumulate{0};
    volatile uint8_t lastMovedCount{ENC_ACCEL_START};
#ifndef WITHOUT_BUTTON
    volatile eButton button;
    uint16_t keyDownTicks{0};
    uint8_t doubleClickTicks{0};
    unsigned long lastButtonCheckCount{0};
#endif
};

#endif // CLICKENCODER_H

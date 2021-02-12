// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Held, LongPressRepeat
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
//
// Refactored and feature added (LongPressRepeat) by Schallbert 2021
// ----------------------------------------------------------------------------

#ifndef __have__ClickEncoder_h__
#define __have__ClickEncoder_h__

// ----------------------------------------------------------------------------

#include <Arduino.h>

// ----------------------------------------------------------------------------

#define ENC_NORMAL (1 << 1) // use Peter Danneger's decoder
#define ENC_FLAKY (1 << 2)  // use Table-based decoder

// ----------------------------------------------------------------------------

#ifndef ENC_DECODER
#define ENC_DECODER ENC_NORMAL
#endif

#if ENC_DECODER == ENC_FLAKY
#ifndef ENC_HALFSTEP
#define ENC_HALFSTEP 1 // use table for half step per default
#endif
#endif

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

#endif // __have__ClickEncoder_h__

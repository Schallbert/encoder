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

#include "ClickEncoder.h"

// ----------------------------------------------------------------------------
// eButton configuration (values for 1ms timer service calls)
//
constexpr uint8_t ENC_BUTTONINTERVAL = 20;            // check button every x ms, also debouce time
constexpr uint16_t ENC_DOUBLECLICKTIME = 400;         // second click within x ms
constexpr uint16_t ENC_LONGPRESSREPEATINTERVAL = 200; // reports repeating-held every x ms
constexpr uint16_t ENC_HOLDTIME = 1200;               // report held button after x ms

// ----------------------------------------------------------------------------
// Acceleration configuration (for 1000Hz calls to ::service())
//
constexpr uint8_t ENC_ACCEL_START = 64; // Start increasing count > (1/tick) below tick iterval of x ms.
constexpr uint8_t ENC_ACCEL_SLOPE = 16;  // below ACCEL_START, velocity progression of 1/x ticks
// Example: increment every 100ms, without acceleration it would count to 10 within 1sec.
// With acceleration START@200 and slope@25 it will count to 50 within 1 sec.

// ----------------------------------------------------------------------------

#if ENC_DECODER != ENC_NORMAL
#ifdef ENC_HALFSTEP
// decoding table for hardware with flaky notch (half resolution)
const int8_t ClickEncoder::table[16] __attribute__((__progmem__)) = {
    0, 0, -1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, -1, 0, 0};
#else
// decoding table for normal hardware
const int8_t ClickEncoder::table[16] __attribute__((__progmem__)) = {
    0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};
#endif
#endif

// ----------------------------------------------------------------------------

ClickEncoder::ClickEncoder(
    uint8_t A,
    uint8_t B,
    uint8_t BTN,
    uint8_t stepsPerNotch,
    bool active) : pinA(A),
                   pinB(B),
                   pinBTN(BTN),
                   stepsPerNotch(stepsPerNotch),
                   pinsActive(active),
                   doubleClickEnabled(false),
                   longPressRepeatEnabled(false),
                   accelerationEnabled(false),
                   lastEncoderRead(0),
                   encoderAccumulate(0),
                   lastEncoderAccumulate(0),
                   lastMoved(ENC_ACCEL_START),
                   button(Open)
{
    uint8_t configType = (pinsActive == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(pinA, configType);
    pinMode(pinB, configType);
    pinMode(pinBTN, configType);
}

// ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//
void ClickEncoder::service(void)
{
    handleEncoder();
#ifndef WITHOUT_BUTTON
    handleButton();
#endif // WITHOUT_BUTTON
}

// ----------------------------------------------------------------------------

void ClickEncoder::handleEncoder()
{

#if ENC_DECODER == ENC_FLAKY
    //TODO: refactor
    lastEncoderRead = (lastEncoderRead << 2) & 0x0F;

    if (digitalRead(pinA) == pinsActive)
    {
        lastEncoderRead |= 2;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        lastEncoderRead |= 1;
    }

    uint8_t tbl = pgm_read_byte(&table[lastEncoderRead]);
    if (tbl)
    {
        encoderValue += tbl;
        moved = true;
    }
#elif ENC_DECODER == ENC_NORMAL
    // bit0 of this indicates if the status has changed
    // bit1 indicates an "overflow 3" where it goes 0->3 or 3->0

    uint8_t encoderRead = getBitCode();
    uint8_t rawMovement = encoderRead - lastEncoderRead;
    // This is the magic, converts raw to: -1 counterclockwise, 0 no, 1 clockwise
    int8_t signedMovement = ((rawMovement & 1) - (rawMovement & 2));
    lastEncoderRead = encoderRead;

    // encoderMovement will now show:
    // 0 when not turned
    // -1 turned anticlockwise
    // 1 turned clockwise
#else
#error "Error: define ENC_DECODER to ENC_NORMAL or ENC_FLAKY"
#endif // ENC_FLAKY
    encoderAccumulate += handleValues(signedMovement);
}

int8_t ClickEncoder::handleValues(int8_t moved)
{

    ++lastMoved;
    if (lastMoved >= ENC_ACCEL_START)
    {
        lastMoved = ENC_ACCEL_START;
    }

    if (moved == 0)
    {
        return 0;
    }

    if (!accelerationEnabled)
    {
        return moved;
    }
    else
    {
        uint8_t acceleration{(ENC_ACCEL_START - lastMoved) / ENC_ACCEL_SLOPE};
        lastMoved = 0;
        if (moved > 0)
        {
            return acceleration;
        }
        else
        {
            return -acceleration;
        }
    }
}

uint8_t ClickEncoder::getBitCode()
{
    // GrayCode convert
    // !A && !B --> 0
    // !A && B --> 1
    // A && B --> 2
    // A && !B --> 3
    uint8_t currentEncoderRead{0};

    if (digitalRead(pinA))
    {
        currentEncoderRead = 3;
    }

    // invert result's 0th bit if set
    currentEncoderRead ^= digitalRead(pinB);
    return currentEncoderRead;
}

// ----------------------------------------------------------------------------

int16_t ClickEncoder::getIncrement()
{
    int16_t result = encoderAccumulate - lastEncoderAccumulate;
    lastEncoderAccumulate = encoderAccumulate;

    // divide value by stepsPerNotch
    result = result / stepsPerNotch;
    return result;
}

int16_t ClickEncoder::getAccumulate()
{
    return (encoderAccumulate / stepsPerNotch);
}

// ----------------------------------------------------------------------------

void ClickEncoder::handleButton()
{
    if (pinBTN == 0)
    {
        return; // button pin not defined
    }

    unsigned long now = millis();
    if ((now - lastButtonCheck) < ENC_BUTTONINTERVAL) // checking button is sufficient every 10-30ms
    {
        return;
    }
    lastButtonCheck = now;

    if (digitalRead(pinBTN) == pinsActive)
    {
        handleButtonPressed();
    }
    else
    {
        handleButtonReleased();
    }

    if (doubleClickTicks > 0)
    {
        --doubleClickTicks;
    }
}

void ClickEncoder::handleButtonPressed()
{
    button = Closed;
    keyDownTicks++;
    if (keyDownTicks >= (ENC_HOLDTIME / ENC_BUTTONINTERVAL))
    {
        button = Held;
        if (!longPressRepeatEnabled)
        {
            return;
        }

        // Blip out LongPressRepeat once per interval
        if (keyDownTicks > ((ENC_LONGPRESSREPEATINTERVAL + ENC_HOLDTIME) / ENC_BUTTONINTERVAL))
        {
            button = LongPressRepeat;
        }
    }
}

void ClickEncoder::handleButtonReleased()
{
    keyDownTicks = 0;
    if (button == Held)
    {
        button = Released;
    }
    else if (button == Closed)
    {
        button = Clicked;
        if (!doubleClickEnabled)
        {
            return;
        }

        if (doubleClickTicks == 0)
        {
            // set counter and wait for another click
            doubleClickTicks = (ENC_DOUBLECLICKTIME / ENC_BUTTONINTERVAL);
        }
        else
        {
            //doubleclick active and not elapsed!
            button = DoubleClicked;
            doubleClickTicks = 0;
        }
    }
}

#ifndef WITHOUT_BUTTON
ClickEncoder::eButton ClickEncoder::getButton(void)
{
    volatile ClickEncoder::eButton result = button;
    if (result == LongPressRepeat)
    {
        // Reset to "Held"
        keyDownTicks = (ENC_HOLDTIME / ENC_BUTTONINTERVAL);
    }

    // reset after readout. Conditional to neither miss nor repeat DoubleClicks or Helds
    if (button != Closed)
    {
        button = Open;
    }

    return result;
}
#endif // WITHOUT_BUTTON

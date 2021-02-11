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
constexpr uint8_t ENC_ACCEL_START = 200; // Start increasing count > (1/tick) below tick iterval of x ms
constexpr uint8_t ENC_ACCEL_SLOPE = 25;  // below ACCEL_START, velocity progression of 1/x ticks
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
                   steps(stepsPerNotch),
                   pinsActive(active),
                   doubleClickEnabled(false),
                   longPressRepeatEnabled(false),
                   accelerationEnabled(false),
                   encoderValue(0),
                   lastEncoderRead(0),
                   acceleration(0),
                   lastMoved(ENC_ACCEL_START),
                   button(Open)
{
    uint8_t configType = (pinsActive == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(pinA, configType);
    pinMode(pinB, configType);
    pinMode(pinBTN, configType);

    if (digitalRead(pinA) == pinsActive)
    {
        lastEncoderRead = 3;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        lastEncoderRead ^= 1;
    }
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
    bool moved = false;

#if ENC_DECODER == ENC_FLAKY
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
    int8_t currentEncoderRead = 0;

    if (digitalRead(pinA) == pinsActive)
    {
        currentEncoderRead = 3;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        currentEncoderRead ^= 1;
    }

    int8_t encoderMovement = lastEncoderRead - currentEncoderRead;
    
    // bit 0 = step
    if (encoderMovement & 1)
    { 
        lastEncoderRead = currentEncoderRead;
        encoderValue += (encoderMovement & 2) - 1; // bit 1 = direction (+/-)
        moved = true;
    }
#else
#error "Error: define ENC_DECODER to ENC_NORMAL or ENC_FLAKY"
#endif // ENC_FLAKY
    if (accelerationEnabled)
    {
        handleAcceleration();

        if (moved)
        {
            lastMoved = 0;
        }
    }
}

void ClickEncoder::handleAcceleration()
{
    ++lastMoved;
    if (lastMoved >= ENC_ACCEL_START)
    {
        lastMoved = ENC_ACCEL_START;
        acceleration = 0;
    }
    else
    {
        acceleration = (ENC_ACCEL_START - lastMoved) / ENC_ACCEL_SLOPE;
    }
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

// ----------------------------------------------------------------------------

int16_t ClickEncoder::getValue(void)
{
    int16_t val;

    cli();
    val = encoderValue;

    if (steps == 2)
        encoderValue = val & 1;
    else if (steps == 4)
        encoderValue = val & 3;
    else
        encoderValue = 0; // default to 1 step per notch
    sei();

    if (steps == 4)
        val >>= 2;
    if (steps == 2)
        val >>= 1;

    int16_t r = 0;

    if (val < 0)
    {
        r -= 1 + acceleration;
    }
    else if (val > 0)
    {
        r += 1 + acceleration;
    }

    return r;
}

// ----------------------------------------------------------------------------

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

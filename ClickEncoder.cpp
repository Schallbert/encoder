// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Long Click
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
// ----------------------------------------------------------------------------

#include "ClickEncoder.h"

// ----------------------------------------------------------------------------
// eButton configuration (values for 1ms timer service calls)
//
#define ENC_BUTTONINTERVAL 20           // check button every x milliseconds, also debouce time
#define ENC_DOUBLECLICKTIME 400         // second click within 400ms
#define ENC_LONGPRESSREPEATINTERVAL 200 // reports repeating-held every x ms
#define ENC_HOLDTIME 1200               // report held button after 1.2s

// ----------------------------------------------------------------------------
// Acceleration configuration (for 1000Hz calls to ::service())
//
#define ENC_ACCEL_TOP 3072 // max. acceleration: *12 (val >> 8)
#define ENC_ACCEL_INC 25
#define ENC_ACCEL_DEC 2

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
                   delta(0),
                   last(0),
                   acceleration(0),
                   button(Open)
{
    uint8_t configType = (pinsActive == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(pinA, configType);
    pinMode(pinB, configType);
    pinMode(pinBTN, configType);

    if (digitalRead(pinA) == pinsActive)
    {
        last = 3;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        last ^= 1;
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
    last = (last << 2) & 0x0F;

    if (digitalRead(pinA) == pinsActive)
    {
        last |= 2;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        last |= 1;
    }

    uint8_t tbl = pgm_read_byte(&table[last]);
    if (tbl)
    {
        delta += tbl;
        moved = true;
    }
#elif ENC_DECODER == ENC_NORMAL
    int8_t curr = 0;

    if (digitalRead(pinA) == pinsActive)
    {
        curr = 3;
    }

    if (digitalRead(pinB) == pinsActive)
    {
        curr ^= 1;
    }

    int8_t diff = last - curr;

    if (diff & 1)
    { // bit 0 = step
        last = curr;
        delta += (diff & 2) - 1; // bit 1 = direction (+/-)
        moved = true;
    }
#else
#error "Error: define ENC_DECODER to ENC_NORMAL or ENC_FLAKY"
#endif

    if (accelerationEnabled)
    {
        handleAcceleration(moved);
    }
}

void ClickEncoder::handleAcceleration(bool hasMoved)
{
    // decelerate every tick
    acceleration -= ENC_ACCEL_DEC;
    if (acceleration & 0x8000)
    {
        acceleration = 0; // handle overflow of MSB is set
    }

    if (hasMoved)
    {
        // increment accelerator if encoder has been moved
        if (acceleration <= (ENC_ACCEL_TOP - ENC_ACCEL_INC))
        {
            acceleration += ENC_ACCEL_INC;
        }
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
    val = delta;

    if (steps == 2)
        delta = val & 1;
    else if (steps == 4)
        delta = val & 3;
    else
        delta = 0; // default to 1 step per notch
    sei();

    if (steps == 4)
        val >>= 2;
    if (steps == 2)
        val >>= 1;

    int16_t r = 0;
    int16_t accel = ((accelerationEnabled) ? (acceleration >> 8) : 0);

    if (val < 0)
    {
        r -= 1 + accel;
    }
    else if (val > 0)
    {
        r += 1 + accel;
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
#endif

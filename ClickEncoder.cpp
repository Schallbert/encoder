// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Held, LongPressRepeat
//
// Refactored, logic upgraded and feature added (LongPressRepeat) by Schallbert 2021
// ----------------------------------------------------------------------------

#include "ClickEncoder.h"

// ----------------------------------------------------------------------------

/* ClickEncoders typically have 5 pins, A, B, C (GND), BTN, GND
// Most of them have notches and register 4 steps (ticks) per notch.
// If the encoder has no button, don't give it a PIN number.
*/
Encoder::Encoder(uint8_t A,
                 uint8_t B,
                 uint8_t stepsPerNotch = 4,
                 bool active = LOW) : pinA(A),
                                      pinB(B),
                                      stepsPerNotch(stepsPerNotch),
                                      pinActiveState(active)
{
    uint8_t configType = (pinActiveState == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(pinA, configType);
    pinMode(pinB, configType);
}

Button::Button(uint8_t BTN = -1,
               bool active = LOW) : pinBTN(BTN),
                                    pinActiveState(active)
{
    uint8_t configType = (pinActiveState == LOW) ? INPUT_PULLUP : INPUT;
    pinMode(pinBTN, configType);
}

ClickEncoder::ClickEncoder(
    uint8_t A,
    uint8_t B,
    uint8_t BTN,
    uint8_t stepsPerNotch,
    bool active)
{
    enc = new Encoder(A, B, stepsPerNotch, active);
    btn = new Button(BTN, active);
}

ClickEncoder::~ClickEncoder()
{
    delete enc;
    delete btn;
    enc = nullptr;
    btn = nullptr;
}

// ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//
void ClickEncoder::service(void)
{
    enc->service();
    btn->service();
}

void Encoder::service()
{
    handleEncoder();
}

void Button::service()
{
    handleButton();
}

// ----------------------------------------------------------------------------

void Encoder::handleEncoder()
{
    uint8_t encoderRead = getBitCode();
    // bit0 set = status changed, bit1 set = "overflow 3" where it goes 0->3 or 3->0
    uint8_t rawMovement = encoderRead - lastEncoderRead;
    lastEncoderRead = encoderRead;
    // This is the magic, converts raw to: -1 counterclockwise, 0 no turn, 1 clockwise
    int8_t signedMovement = ((rawMovement & 1) - (rawMovement & 2));

    encoderAccumulate += handleValues(signedMovement);
}

uint8_t Encoder::getBitCode()
{
    // GrayCode convert
    // !A && !B --> 0
    // !A &&  B --> 1
    //  A &&  B --> 2
    //  A && !B --> 3
    uint8_t currentEncoderRead{0};
    if (digitalRead(pinA))
    {
        currentEncoderRead = 3;
    }
    // invert result's 0th bit if set
    currentEncoderRead ^= digitalRead(pinB);
    return currentEncoderRead;
}

int8_t Encoder::handleValues(int8_t moved)
{
    if (lastMovedCount < ENC_ACCEL_START)
    {
        ++lastMovedCount;
    }

    if (moved == 0 || !accelerationEnabled)
    {
        return moved;
    }
    else
    {
        lastMovedCount = 0;
        // moved && acceleration enabled
        uint8_t acceleration{(ENC_ACCEL_START - lastMovedCount) / ENC_ACCEL_SLOPE};
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
// ----------------------------------------------------------------------------

int16_t Encoder::getIncrement()
{
    int16_t encoderIncrements = encoderAccumulate - lastEncoderAccumulate;
    lastEncoderAccumulate = encoderAccumulate;
    return (encoderIncrements / stepsPerNotch);
}

int16_t Encoder::getAccumulate()
{
    return (encoderAccumulate / stepsPerNotch);
}

// ----------------------------------------------------------------------------
void Button::handleButton()
{
    if (pinBTN == 0)
    {
        return; // button pin not defined
    }

    unsigned long now = millis();
    if ((now - lastButtonCheckCount) < ENC_BUTTONINTERVAL) // checking button is sufficient every 10-30ms
    {
        return;
    }
    lastButtonCheckCount = now;

    if (digitalRead(pinBTN) == pinActiveState)
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

void Button::handleButtonPressed()
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

void Button::handleButtonReleased()
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

Button::eButtonStates Button::getButton(void)
{
    volatile Button::eButtonStates result = button;
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
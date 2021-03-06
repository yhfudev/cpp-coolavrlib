/**
 * @file    button.cpp
 * @brief   Button Click Lib for Arduino, supports 1 click, double clicks, long press, very long press
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */

#include "sysport.h"
#include "button.h"

/**
TODO:
timer for timeout
key interrupt
lock-free queues for both timer and key interrupt
timeout types for states
cancel timer: in both queue and timer


interfaces:
on_timer_add(userdata,time_ms), userdata=state_type
on_timer_cancel(id)
timer_expired:(userdata); -- create Event TIMEOUT_XX

register_key_interrupt
key_changed -- create Event PRESSED/RELEASED

main loop to process the queues



timer in ATmega168/328
TIMER0 -- 8bit, is used by native Arduino timing functions such as delay() and millis().
TIMER1 -- 16bit, is used by a Arduino Servo library.
TIMER2 -- 8bit, is utilized by the Arduino tone()

ATmega1280/2560
TIMER1, TIMER2, TIMER3 -- identical to ATmega168/328's
TIMER3, TIMER4, TIMER5 -- 16bit


Arduino 101: Timers and Interrupts
https://arduino-info.wikispaces.com/Timers-Arduino
http://letsmakerobots.com/content/arduino-101-timers-and-interrupts

Arduino ATmega328
Pins 5 and 6: controlled by timer0
Pins 9 and 10: controlled by timer1
Pins 11 and 3: controlled by timer2

Arduino Mega
Pins 4 and 13: controlled by timer0
Pins 11 and 12: controlled by timer1
Pins 9 and10: controlled by timer2
Pin 2, 3 and 5: controlled by timer 3
Pin 6, 7 and 8: controlled by timer 4
Pin 46, 45 and 44:: controlled by timer 5

Microcontroller tutorial series: AVR and Arduino timer interrupts
http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/


Timer 1 and 3:
http://playground.arduino.cc/Code/Timer1
https://github.com/PaulStoffregen/TimerOne.git
https://github.com/PaulStoffregen/TimerThree.git
*/

#ifndef TRACE
#define TRACE(...)
#endif

// 0 -- verbose, 1 -- info, 2 -- warning, 3 -- error
#define TRACE0 TRACE
#define TRACE1 TRACE
#define TRACE2 TRACE
#define TRACE3 TRACE

#if 1
#undef TRACE0
#define TRACE0(...)
//#undef TRACE1
//#define TRACE1(...)
#undef TRACE2
#define TRACE2(...)
//#undef TRACE3
//#define TRACE3(...)
#endif

// internal states
#define BUTSW_STATE_READY      0
#define BUTSW_STATE_DEBOUNCE   1
#define BUTSW_STATE_1CLICK     2
#define BUTSW_STATE_LONGPRESS  3
#define BUTSW_STATE_VLONGPRESS 4
#define BUTSW_STATE_DEBOUNCE2  5

// event types
#define BUTSW_EVT_NONE          0
#define BUTSW_EVT_PRESSED       1
#define BUTSW_EVT_RELEASED      2
#define BUTSW_EVT_TIMEOUT_DB1   3
#define BUTSW_EVT_TIMEOUT_DB2   4
#define BUTSW_EVT_TIMEOUT_CLICK 5
#define BUTSW_EVT_TIMEOUT_LONG  6
#define BUTSW_EVT_TIMEOUT_READY 7
#define BUTSW_EVT_TIMEOUT       8 /* temp */

#if DEBUG
char * val2cstr_buttsw_state(int val)
{
#define CASESTATE(v) case BUTSW_STATE_ ##v: return "STATE_" #v
    switch (val) {
        CASESTATE(READY);
        CASESTATE(DEBOUNCE);
        CASESTATE(1CLICK);
        CASESTATE(LONGPRESS);
        CASESTATE(VLONGPRESS);
        CASESTATE(DEBOUNCE2);
    }
    return "STATE_(unknow)";
#undef CASESTATE
}
char * val2cstr_buttsw_evt(int val)
{
#define CASEEVT(v) case BUTSW_EVT_ ##v: return "EVT_" #v
    switch (val) {
        CASEEVT(NONE);
        CASEEVT(TIMEOUT);
        CASEEVT(PRESSED);
        CASEEVT(RELEASED);
    }
    return "EVT_(unknow)";
#undef CASEEVT
}
#define VAL2CSTR_BUTTSW_STATE(v) val2cstr_buttsw_state(v)
#define VAL2CSTR_BUTTSW_EVT(v) val2cstr_buttsw_evt(v)
#else
#define VAL2CSTR_BUTTSW_STATE(v) "val2cstr_buttsw_state unimplemented"
#define VAL2CSTR_BUTTSW_EVT(v) "val2cstr_buttsw_evt unimplemented"
#endif // DEBUG

Button::Button(bool multiple_click1)
{
    this->pin = 0;
    this->current_state = BUTSW_STATE_READY;
    this->button_hold = false;
    this->released_state = HIGH;
    this->clicks = 0;
    this->multiple_click = multiple_click1;

    this->userdata = nullptr;
    this->OnClick = nullptr;
    this->OnLongPress = nullptr;
    this->OnVLongPress = nullptr;
    this->OnStart = nullptr;
    this->OnEnd = nullptr;
}

bool
Button::is_pressed(uint8_t cur_state)
{
    if (this->released_state == cur_state) {
        return false;
    } else {
        return true;
    }
}

void
Button::set_pin (uint8_t digital_pin, uint8_t pressed_state)
{
    if (pressed_state == HIGH) {
        this->released_state = LOW;
    } else {
        this->released_state = HIGH;
    }
    this->pin = digital_pin;
    this->clicks = 0;
    TRACE0 ("Button: set_pin: pin=%d, pressed_state=%d", this->pin, pressed_state);
}

void
Button::set_pin (uint8_t digital_pin)
{
    if (digitalRead(digital_pin)) {
        set_pin (digital_pin, LOW);
    } else {
        set_pin (digital_pin, HIGH);
    }
}

// start a timer with timeout time_ms
// when timeout, push a Event to the state machine
// the timer is simulated by calling Button::update() in the main loop()
void
Button::start_timer (int time_ms)
{
    this->timer_prev = millis();
    this->timer_len  = time_ms;
    this->timer_accu = 0;
}

void
Button::cancle_timer ()
{
    this->timer_len = 0;
}

// check the time and return 1 if expired, -1 on error, 0 on normal
int
Button::update_timer ()
{
    if (this->timer_len < 1) {
        return -1;
    }
    unsigned long now = millis();
    unsigned long val = now;
    if (this->timer_prev <= now) {
        val = now - this->timer_prev;
    } else {
        TRACE3 ("millis() rollback!");
        val = now + 10000 - this->timer_prev;
    }
    this->timer_accu += val;
    this->timer_prev = now;
    if (this->timer_accu >= this->timer_len) {
        this->timer_len = 0;
        return 1;
    }
    return 0;
}

uint8_t
Button::get_key_type (void)
{
    switch (this->current_state) {
    case BUTSW_STATE_READY:
    case BUTSW_STATE_DEBOUNCE:
    case BUTSW_STATE_DEBOUNCE2:
        if (this->clicks > 1) {
            return BUTSW_TYPE_2CLICK;
        }
        if (this->clicks > 0) {
            return BUTSW_TYPE_1CLICK;
        }
        break;

        if (this->clicks > 1) {
            return BUTSW_TYPE_2CLICK;
        }
        if (this->clicks > 0) {
            return BUTSW_TYPE_1CLICK;
        }
        break;

    case BUTSW_STATE_1CLICK:
        if (this->clicks > 0) {
            return BUTSW_TYPE_2CLICK;
        }
        return BUTSW_TYPE_1CLICK;

    case BUTSW_STATE_LONGPRESS:
        return BUTSW_TYPE_LONGPRESS;

    case BUTSW_STATE_VLONGPRESS:
        return BUTSW_TYPE_VLONGPRESS;

    default:
        TRACE3 ("Error: unknown state");
        break;
    }
    return BUTSW_TYPE_NONE;
}

uint8_t
Button::process_event (Button::Event &ev)
{
    uint8_t next_state = this->current_state;
    TRACE1 ("Button: %s on %s", VAL2CSTR_BUTTSW_STATE(this->current_state), VAL2CSTR_BUTTSW_EVT(ev.get_type()));
    switch (this->current_state) {
    case BUTSW_STATE_READY:
        switch (ev.get_type()) {
        case BUTSW_EVT_TIMEOUT:
            // check the current button status
            if (this->clicks > 0) {
                TRACE1 ("Button: CB end");
                if (this->OnEnd) {
                    this->OnEnd (this->userdata);
                }
                TRACE1 ("Button: CB 1click");
                if (this->OnClick) {
                    this->OnClick (this->userdata, this->clicks);
                }
                this->clicks = 0;
            }
            break;
        case BUTSW_EVT_PRESSED:
            this->button_hold = true;
            next_state = BUTSW_STATE_DEBOUNCE;
            start_timer (get_timeout_dbounce());
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;

    case BUTSW_STATE_DEBOUNCE:
        switch (ev.get_type()) {
        case BUTSW_EVT_TIMEOUT:
            // check the current button status
            if (this->button_hold) {
                start_timer (get_timeout_long());
                next_state = BUTSW_STATE_1CLICK;
                TRACE1 ("Button: CB start");
                if (this->OnStart) {
                    this->OnStart (this->userdata);
                }
            } else {
                next_state = BUTSW_STATE_READY;
            }
            break;
        case BUTSW_EVT_PRESSED:
            this->button_hold = true;
            break;
        case BUTSW_EVT_RELEASED:
            this->button_hold = false;
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;

    case BUTSW_STATE_1CLICK:
        switch (ev.get_type()) {
        case BUTSW_EVT_TIMEOUT:
            // check the current button status
            if (this->button_hold) {
                start_timer (get_timeout_vlong());
                this->clicks = 0;
                next_state = BUTSW_STATE_LONGPRESS;
                break;
            } else {
                // not possible state
                TRACE1 ("Button: not possible in 1click with button released");
            }
        case BUTSW_EVT_RELEASED:
            this->button_hold = false;
            cancle_timer();
            if (this->multiple_click) {
                this->clicks ++;
            } else {
                this->clicks = 0;
                TRACE1 ("Button: CB onEnd");
                if (this->OnEnd) {
                    this->OnEnd (this->userdata);
                }
                TRACE1 ("Button: CB OnClick");
                if (this->OnClick) {
                    this->OnClick (this->userdata, 1);
                }
            }
            start_timer (get_timeout_dbounce());
            next_state = BUTSW_STATE_DEBOUNCE2;
            break;
        case BUTSW_EVT_PRESSED:
            this->button_hold = true;
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;

    case BUTSW_STATE_LONGPRESS:
        switch (ev.get_type()) {
        case BUTSW_EVT_TIMEOUT:
            // check the current button status
            if (this->button_hold) {
                // callback
                TRACE1 ("Button: CB vlongpress");
                if (this->OnVLongPress) {
                    this->OnVLongPress (this->userdata);
                }
                //next_state = BUTSW_STATE_VLONGPRESS;
                start_timer (get_timeout_dbounce());
                next_state = BUTSW_STATE_DEBOUNCE2;
            } else {
                TRACE1 ("Button: not possible in LONG with button released");
            }
            break;
        case BUTSW_EVT_PRESSED:
            this->button_hold = true;
            break;
        case BUTSW_EVT_RELEASED:
            this->button_hold = false;
            cancle_timer();
            this->clicks = 0;
            // callback
            TRACE1 ("Button: CB onEnd");
            if (this->OnEnd) {
                this->OnEnd (this->userdata);
            }
            TRACE1 ("Button: CB longpress");
            if (this->OnLongPress) {
                this->OnLongPress (this->userdata);
            }
            start_timer (get_timeout_dbounce());
            next_state = BUTSW_STATE_DEBOUNCE2;
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;

    case BUTSW_STATE_VLONGPRESS:
        switch (ev.get_type()) {
        case BUTSW_EVT_PRESSED:
            this->button_hold = true;
            break;
        case BUTSW_EVT_RELEASED:
            cancle_timer();
            this->button_hold = false;
            this->clicks = 0;
            // callback
            TRACE1 ("Button: CB onEnd");
            if (this->OnEnd) {
                this->OnEnd (this->userdata);
            }
            start_timer (get_timeout_dbounce());
            next_state = BUTSW_STATE_DEBOUNCE2;
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;

    case BUTSW_STATE_DEBOUNCE2:
        switch (ev.get_type()) {
        case BUTSW_EVT_TIMEOUT:
            this->button_hold = false;
            cancle_timer();
            if (this->multiple_click && this->clicks > 0) {
                start_timer (get_timeout_2click());
            }
            next_state = BUTSW_STATE_READY;
            break;
        case BUTSW_EVT_PRESSED:
            //this->button_hold = true;
            break;
        case BUTSW_EVT_RELEASED:
            //this->button_hold = false;
            break;
        default:
            TRACE3 ("Button: Unhandled : %s", VAL2CSTR_BUTTSW_EVT(ev.get_type()));
            break;
        }
        break;
    default:
        TRACE3 ("Error: unknown state: %s", VAL2CSTR_BUTTSW_STATE(this->current_state));
        break;
    }
    this->current_state = next_state;
    return next_state;
}

void
Button::update_pin (uint8_t pin_state)
{
    //TRACE1 ("update_pin(%d) ...", pin_state);

    bool is_changed = (this->button_hold != is_pressed(pin_state));
    if (is_changed) {
        Event ev (BUTSW_EVT_PRESSED);
        if (this->button_hold) {
            ev.set_type (BUTSW_EVT_RELEASED);
        }
        this->process_event (ev);
    }
}

void
Button::update_other ()
{
    // check the timer
    if (1 == this->update_timer()) {
        TRACE0 ("Button: Time out!");
        Button::Event ev(BUTSW_EVT_TIMEOUT);
        this->process_event (ev);
    }
}

bool
Button::update ()
{
    bool ret = false;
    update_pin (digitalRead(this->pin));
    update_other ();
    if (this->timer_len > 0) {
        ret = true;
    }
    if (this->current_state != BUTSW_STATE_READY) {
        ret = true;
    }
    return ret;
}


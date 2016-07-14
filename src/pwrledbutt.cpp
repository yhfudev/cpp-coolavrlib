/**
 * @file    pwrledbutt.cpp
 * @brief   Power Button with LED
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */

#include "sysport.h"
#include "pwrledbutt.h"

#ifndef TRACE
#define TRACE(...)
#endif

// 0 -- verbose, 1 -- info, 2 -- warning, 3 -- error
#define TRACE0 TRACE
#define TRACE1 TRACE
#define TRACE2 TRACE
#define TRACE3 TRACE

#if 0
#undef TRACE0
#undef TRACE1
#undef TRACE2
//#undef TRACE3
#define TRACE0(...)
#define TRACE1(...)
#define TRACE2(...)
//#define TRACE3(...)
#endif

// internal states
#define PWRLEDBUTT_STATE_STANDBY    0
#define PWRLEDBUTT_STATE_ON_RELEASE 1
#define PWRLEDBUTT_STATE_ON         2
#define PWRLEDBUTT_STATE_SHUTDOWN   3

// event types
#define PWRLEDBUTT_EVT_NONE         0  /* Not a valid event */
#define PWRLEDBUTT_EVT_TIMEOUT      1  /* timeout of internal timer */
#define PWRLEDBUTT_EVT_ONBEGIN      2  /* key press down */
#define PWRLEDBUTT_EVT_ONEND        3  /* key press down */
#define PWRLEDBUTT_EVT_ONCLICK      4  /* key clicked */
#define PWRLEDBUTT_EVT_ONLONG       5  /* the long pressed key */
#define PWRLEDBUTT_EVT_ONSIGOFF     6  /* the host send sigal OFF by signal_off() */
#define PWRLEDBUTT_EVT_ONSIGRDY     7  /* the host send sigal READY by signal_ready() */

#if DEBUG
char * val2cstr_pwrledbutt_state(int val)
{
#define CASESTATE(v) case PWRLEDBUTT_STATE_ ##v: return "STATE_" #v
    switch (val) {
        CASESTATE(STANDBY);
        CASESTATE(ON_RELEASE);
        CASESTATE(ON);
        CASESTATE(SHUTDOWN);
    }
    return "STATE_(unknow)";
#undef CASESTATE
}
char * val2cstr_pwrledbutt_evt(int val)
{
#define CASEEVT(v) case PWRLEDBUTT_EVT_ ##v: return "EVT_" #v
    switch (val) {
        CASEEVT(NONE);
        CASEEVT(TIMEOUT);
        CASEEVT(ONBEGIN);
        CASEEVT(ONEND);
        CASEEVT(ONCLICK);
        CASEEVT(ONLONG);
        CASEEVT(ONSIGOFF);
        CASEEVT(ONSIGRDY);
    }
    return "EVT_(unknow)";
#undef CASEEVT
}
#define VAL2CSTR_PWRLEDBUTT_STATE(v) val2cstr_pwrledbutt_state(v)
#define VAL2CSTR_PWRLEDBUTT_EVT(v) val2cstr_pwrledbutt_evt(v)
#else
#define VAL2CSTR_PWRLEDBUTT_STATE(v) "val2cstr_pwrledbutt_state unimplemented"
#define VAL2CSTR_PWRLEDBUTT_EVT(v) "val2cstr_pwrledbutt_evt unimplemented"
#endif // DEBUG

// start a timer with timeout time_ms
// when timeout, push a Event to the state machine
// the timer is simulated by calling Button::update() in the main loop()
void
PowerLedButton::start_timer (int time_ms)
{
    this->timer_prev = millis();
    this->timer_len  = time_ms;
    this->timer_accu = 0;
}

void
PowerLedButton::cancle_timer ()
{
    this->timer_len = 0;
}

// check the time and return 1 if expired, -1 on error, 0 on normal
int
PowerLedButton::update_timer ()
{
    if (this->timer_len < 1) {
        return -1;
    }
    unsigned long now = millis();
    unsigned long val = now;
    if (this->timer_prev <= now) {
        val = now - this->timer_prev;
    }
    this->timer_accu += val;
    this->timer_prev = now;
    if (this->timer_accu >= this->timer_len) {
        this->timer_len = 0;
        return 1;
    }
    return 0;
}

void
PowerLedButton::blink_led (int type)
{
    LEDBlink *plf = &(this->led_pwm);
    this->led_type = type;
    switch (this->led_type) {
    case PWRLEDBUTT_LEDT_WAITON:
        this->led_direction = PWRLEDBUTT_DIR_DOWN;
        plf->start_fade(get_led_time_fade()/3, get_led_bright_sb2(), get_led_bright_sb1());
        TRACE3 ("led fade (%d, %d, %d)", get_led_time_fade()/4, get_led_bright_sb2(), get_led_bright_sb1());
        break;
    case PWRLEDBUTT_LEDT_WAITOFF:
        this->led_direction = PWRLEDBUTT_DIR_DOWN;
        plf->start_fade(get_led_time_fade()/8, get_led_bright_sb2(), get_led_bright_sb1());
        TRACE3 ("led fade (%d, %d, %d)", get_led_time_fade()/8, get_led_bright_sb2(), get_led_bright_sb1());
        break;
    case PWRLEDBUTT_LEDT_STANDBY:
        this->led_direction = PWRLEDBUTT_DIR_DOWN;
        plf->start_fade(get_led_time_fade(), get_led_bright_sb2(), get_led_bright_sb1());
        TRACE3 ("led fade (%d, %d, %d)", get_led_time_fade(), get_led_bright_sb2(), get_led_bright_sb1());
        break;
    case PWRLEDBUTT_LEDT_ON:
        plf->stop();
        plf->set_value (get_led_bright_on());
        break;
    case PWRLEDBUTT_LEDT_OFF:
        plf->stop();
        plf->set_value (0);
        break;
    }
}

void
PowerLedButton::update_led ()
{
    LEDBlink *plf = &(this->led_pwm);
    plf->update();

    if (! plf->is_blinking()) {
        if (! plf->is_busy()) {
            // LED no longer fading, switch direction
            int bright1 = get_led_bright_sb1();
            int bright2 = get_led_bright_sb2();
            if (this->led_direction == PWRLEDBUTT_DIR_UP) {
                bright1 = get_led_bright_sb2();
                bright2 = get_led_bright_sb1();
                this->led_direction = PWRLEDBUTT_DIR_DOWN;
            } else {
                this->led_direction = PWRLEDBUTT_DIR_UP;
            }
            switch (this->led_type) {
            case PWRLEDBUTT_LEDT_WAITON:
                plf->start_fade(get_led_time_fade()/4, bright1, bright2);
                break;
            case PWRLEDBUTT_LEDT_WAITOFF:
                plf->start_fade(get_led_time_fade()/8, bright1, bright2);
                break;
            case PWRLEDBUTT_LEDT_STANDBY:
                plf->start_fade(get_led_time_fade(), bright1, bright2);
                break;
            }
        }
    }
}

void
PowerLedButton::update_butt ()
{
    this->butt.update();
}

void
PowerLedButton::update_other ()
{
    // check the timer
    if (1 == this->update_timer()) {
        TRACE0 ("PowerLedButton: Time out!");
        Button::Event ev(PWRLEDBUTT_EVT_TIMEOUT);
        this->process_event (ev);
    }
}

void
PowerLedButton::update(void)
{
    update_butt();
    update_led();
    update_other ();
}

void
on_butt_begin (void * userdata)
{
    PowerLedButton *pbutt = static_cast<PowerLedButton *>(userdata);
    Button::Event ev(PWRLEDBUTT_EVT_ONBEGIN);
    pbutt->process_event (ev);
}

void
on_butt_end (void * userdata)
{
    PowerLedButton *pbutt = static_cast<PowerLedButton *>(userdata);
    Button::Event ev(PWRLEDBUTT_EVT_ONEND);
    pbutt->process_event (ev);
}

void
on_butt_click (void * userdata, unsigned int times)
{
    PowerLedButton *pbutt = static_cast<PowerLedButton *>(userdata);
    Button::Event ev(PWRLEDBUTT_EVT_ONCLICK);
    pbutt->process_event (ev);
}

void
on_butt_long (void * userdata)
{
    PowerLedButton *pbutt = static_cast<PowerLedButton *>(userdata);
    Button::Event ev(PWRLEDBUTT_EVT_ONLONG);
    pbutt->process_event (ev);
}

PowerLedButton::PowerLedButton()
: userdata(nullptr)
, cb_poweron(nullptr)
, cb_shutdown(nullptr)
, cb_forceoff(nullptr)
{
    butt.set_user_data (this);
    butt.on_start (on_butt_begin);
    butt.on_end (on_butt_end);
    butt.on_click (on_butt_click);
    butt.on_long_press (on_butt_long);
    butt.on_vlong_press (on_butt_long);
    led_type = PWRLEDBUTT_LEDT_NONE;
}

// told the class that host is off now
void
PowerLedButton::signal_off (void)
{
    Button::Event ev(PWRLEDBUTT_EVT_ONSIGOFF);
    this->process_event (ev);
}

// told the class that host is on and ready
void
PowerLedButton::signal_ready (void)
{
    Button::Event ev(PWRLEDBUTT_EVT_ONSIGRDY);
    this->process_event (ev);
}

uint8_t
PowerLedButton::process_event (Button::Event &ev)
{
    uint8_t next_state = this->current_state;
    TRACE0 ("PowerLedButton: %s on %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
    switch (this->current_state) {
    case PWRLEDBUTT_STATE_STANDBY:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONBEGIN:
            TRACE0 ("PowerLedButton: CB poweron");
            if (this->cb_poweron) {
                this->cb_poweron (this->userdata);
            }
            blink_led (PWRLEDBUTT_LEDT_WAITON);
            next_state = PWRLEDBUTT_STATE_ON_RELEASE;
            break;
        case PWRLEDBUTT_EVT_ONEND:
            break;
        case PWRLEDBUTT_EVT_ONCLICK:
        case PWRLEDBUTT_EVT_ONLONG:
            next_state = PWRLEDBUTT_STATE_ON;
            break;
        case PWRLEDBUTT_EVT_ONSIGOFF:
            //blink_led (PWRLEDBUTT_LEDT_STANDBY);
            //next_state = PWRLEDBUTT_STATE_STANDBY;
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            next_state = PWRLEDBUTT_STATE_ON;
            break;
        default:
            TRACE3 ("PowerLedButton: Unhandled : %s", VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_ON_RELEASE:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONEND:
            break;
        case PWRLEDBUTT_EVT_ONCLICK:
        case PWRLEDBUTT_EVT_ONLONG:
            next_state = PWRLEDBUTT_STATE_ON;
            break;
        case PWRLEDBUTT_EVT_ONSIGOFF:
            //blink_led (PWRLEDBUTT_LEDT_STANDBY);
            //next_state = PWRLEDBUTT_STATE_STANDBY;
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            next_state = PWRLEDBUTT_STATE_ON;
            break;
        default:
            TRACE3 ("PowerLedButton: Unhandled : %s", VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_ON:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONLONG:
            TRACE0 ("PowerLedButton: CB forced off");
            if (this->cb_forceoff) {
                this->cb_forceoff (this->userdata);
            }
            blink_led (PWRLEDBUTT_LEDT_STANDBY);
            next_state = PWRLEDBUTT_STATE_STANDBY;
            break;
        case PWRLEDBUTT_EVT_ONCLICK:
            TRACE0 ("PowerLedButton: CB shutdown");
            if (this->cb_shutdown) {
                this->cb_shutdown (this->userdata);
            }
            start_timer (get_timeout_shutdown());
            blink_led (PWRLEDBUTT_LEDT_WAITOFF);
            next_state = PWRLEDBUTT_STATE_SHUTDOWN;
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            //next_state = PWRLEDBUTT_STATE_ON;
            break;
        case PWRLEDBUTT_EVT_ONSIGOFF:
            //blink_led (PWRLEDBUTT_LEDT_STANDBY);
            next_state = PWRLEDBUTT_STATE_STANDBY;
            break;
        default:
            TRACE3 ("PowerLedButton: Unhandled : %s", VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_SHUTDOWN:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONSIGOFF:
        case PWRLEDBUTT_EVT_TIMEOUT:
        case PWRLEDBUTT_EVT_ONLONG:
            TRACE0 ("PowerLedButton: CB forced off");
            if (this->cb_forceoff) {
                this->cb_forceoff (this->userdata);
            }
            blink_led (PWRLEDBUTT_LEDT_STANDBY);
            next_state = PWRLEDBUTT_STATE_STANDBY;
            break;
        default:
            TRACE3 ("PowerLedButton: Unhandled : %s", VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    default:
        TRACE3 ("Error: unknown state: %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state));
        break;
    }
    this->current_state = next_state;
    return next_state;
}


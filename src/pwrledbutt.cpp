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
#define PWRLEDBUTT_STATE_NONE       0
#define PWRLEDBUTT_STATE_STANDBY    1
#define PWRLEDBUTT_STATE_BOOT_RELEASE 2
#define PWRLEDBUTT_STATE_BOOT_WAIT  3
#define PWRLEDBUTT_STATE_ON         4
#define PWRLEDBUTT_STATE_SHUTDOWN   6

// event types
#define PWRLEDBUTT_EVT_NONE          0  /* Not a valid event */
#define PWRLEDBUTT_EVT_TIMEOUT_SHUTDOWN 1  /* timeout of internal timer */
#define PWRLEDBUTT_EVT_ONBEGIN       2  /* key press down */
#define PWRLEDBUTT_EVT_ONEND         3  /* key press down */
#define PWRLEDBUTT_EVT_ONCLICK       4  /* key clicked */
#define PWRLEDBUTT_EVT_ONLONG        5  /* the long pressed key */
#define PWRLEDBUTT_EVT_ONSIGOFF      6  /* the host send sigal OFF by signal_off() */
#define PWRLEDBUTT_EVT_ONSIGRDY      7  /* the host send sigal READY by signal_ready() */
#define PWRLEDBUTT_EVT_ENTER_STANDBY 8
#define PWRLEDBUTT_EVT_TIMEOUT_SLEEP1 9  /* timeout of sleep 1 */
#define PWRLEDBUTT_EVT_TIMEOUT_SLEEP2 10  /* timeout of sleep 2 */

#if DEBUG
static char *
val2cstr_pwrledbutt_state(uint_t val)
{
#define CASESTATE(v) case PWRLEDBUTT_STATE_ ##v: return "STATE_" #v
    switch (val) {
        CASESTATE(NONE);
        CASESTATE(STANDBY);
        CASESTATE(BOOT_RELEASE);
        CASESTATE(BOOT_WAIT);
        CASESTATE(ON);
        CASESTATE(SHUTDOWN);
    }
    return "STATE_(unknow)";
#undef CASESTATE
}

static char *
val2cstr_pwrledbutt_evt(uint_t val)
{
#define CASEEVT(v) case PWRLEDBUTT_EVT_ ##v: return "EVT_" #v
    switch (val) {
        CASEEVT(NONE);
        CASEEVT(TIMEOUT_SHUTDOWN);
        CASEEVT(ONBEGIN);
        CASEEVT(ONEND);
        CASEEVT(ONCLICK);
        CASEEVT(ONLONG);
        CASEEVT(ONSIGOFF);
        CASEEVT(ONSIGRDY);
        CASEEVT(ENTER_STANDBY);
        CASEEVT(TIMEOUT_SLEEP1);
        CASEEVT(TIMEOUT_SLEEP2);
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

void
PowerLedButton::blink_led (int type)
{
    switch (type) {
    case PWRLEDBUTT_LEDT_WAITON:
        // fade.blink(a,b,c): sets the time the led is fully ON, the time the led is fully OFF and the number of repeats.
        // fade.fade( int fade ); Sets the speed time each fade step takes in milliseconds
        // fadeled.blink( uint32_t duration, uint32_t pause_duration, uint16_t repeat_count = ATM_COUNTER_OFF )
        // fadeled.fade( int fade ), the speed time each fade step takes in milliseconds. 32 steps
        TRACE0("set led WAITON");
        this->led.blink( 1, 1 ).fade(1024, 0, 255);//fade(10);
        this->bit_fade.trigger( this->bit_fade.EVT_ON );
        break;
    case PWRLEDBUTT_LEDT_WAITOFF:
        TRACE0("set led WAITOFF");
        this->led.blink( 1, 1 ).fade(512, 0, 255);//fade(5);
        this->bit_fade.trigger( this->bit_fade.EVT_ON );
        break;
    case PWRLEDBUTT_LEDT_STANDBY:
        TRACE0("set led STANDBY");
        this->led.blink( 1, 1 ).fade(2300, 0, 88);//fade(25);
        this->bit_fade.trigger( this->bit_fade.EVT_ON );
        break;
    case PWRLEDBUTT_LEDT_ON:
        TRACE0("set led ON");
        this->bit_butt.trigger( this->bit_fade.EVT_ON );
        break;
    case PWRLEDBUTT_LEDT_OFF:
        TRACE0("set led OFF");
        this->bit_fade.trigger( this->bit_fade.EVT_OFF );
        this->bit_butt.trigger( this->bit_butt.EVT_OFF );
        break;
    }
}

PowerLedButton::PowerLedButton()
: userdata(nullptr)
, cb_poweron(nullptr)
, cb_shutdown(nullptr)
, cb_forceoff(nullptr)
{
}

void
PowerLedButton::setup(void)
{
    // INPUT_PULLUP mode
    // callback(int idx, int v, int up):
    //   idx--the value passed by the second parameter of onPress( callback, <idx> );
    //     v--1 if the event is a button press, other if longpress
    //    up--The direction in which the threshold was crossed (1 = up, 0 = down)
    this->butt.debounce ( 20 )
              //.onPress   ( this->bit_butt, this->bit_butt.EVT_ON )
              //.onRelease ( this->bit_butt, this->bit_butt.EVT_OFF )
              .longPress( 40, 50 )
              .onPress (
                  //pwrbutt_cb_butt_pressed
[]( int idx, int v, int up )
{
    PowerLedButton * pthis;
    pthis = (PowerLedButton *)idx;
    if (v < 0) {
        if (v == -1) {
            TRACE0 ("ATM button down");
            pthis->bit_butt.trigger( pthis->bit_butt.EVT_ON );
            // onbegin
            StateMachine::Event ev(PWRLEDBUTT_EVT_ONBEGIN);
            pthis->add_event (ev);
        }
    } else if (v > 0) {
        // onend
        TRACE0 ("ATM button up");
        pthis->bit_butt.trigger( pthis->bit_butt.EVT_OFF );
        StateMachine::Event ev(PWRLEDBUTT_EVT_ONEND);
        pthis->add_event (ev);

        // 1click, long, vlong:
        if (v < 30) {
            // 1 click
            TRACE0 ("ATM button 1click");
            StateMachine::Event ev(PWRLEDBUTT_EVT_ONCLICK);
            pthis->add_event (ev);
        } else if (v < 100) {
            // long
            TRACE0 ("ATM button long");
            StateMachine::Event ev(PWRLEDBUTT_EVT_ONLONG);
            pthis->add_event (ev);
        } else {
            // vlong
            TRACE0 ("ATM button vlong");
            StateMachine::Event ev(PWRLEDBUTT_EVT_ONLONG);
            pthis->add_event (ev);
        }
    }
}
                  , (int)this)
        ;

    this->bit_butt.begin();
    this->bit_fade.begin();
    this->ctrl_1.begin()
                .IF(bit_butt, '=', bit_butt.ON)
                .onChange (true, this->led, this->led.EVT_ON );
    this->ctrl_2.begin()
                .IF(bit_butt, '=', bit_butt.OFF).AND(bit_fade, '=', bit_fade.ON)
                .onChange (true, this->led, this->led.EVT_BLINK ); // by other functions: bit_fade.trigger(ON); led.blink( 500, 500 ).trigger( led.EVT_BLINK );
    this->ctrl_3.begin()
                .IF(bit_butt, '=', bit_butt.OFF).AND(bit_fade, '=', bit_fade.OFF)
                .onChange (true, this->led, this->led.EVT_OFF );

#if DEBUG && defined(ARDUINO)
    //this->butt.trace (Serial);
    //this->timer.trace (Serial);
    this->led.trace (Serial);
#endif
     // start the standby state
     enter_standby();
}

// told the class that host is off now
void
PowerLedButton::signal_off (void)
{
    StateMachine::Event ev(PWRLEDBUTT_EVT_ONSIGOFF);
    this->add_event (ev);
}

// told the class that host is on and ready
void
PowerLedButton::signal_ready (void)
{
    StateMachine::Event ev(PWRLEDBUTT_EVT_ONSIGRDY);
    this->add_event (ev);
}

// call this function when switch from another state other than standby
void
PowerLedButton::enter_standby (void)
{
     // start the standby state
     blink_led (PWRLEDBUTT_LEDT_STANDBY);
     this->state_current = PWRLEDBUTT_STATE_STANDBY;
     StateMachine::Event ev(PWRLEDBUTT_EVT_ENTER_STANDBY);
     this->add_event (ev);
}

#define SWITCH_STANDBY() enter_standby()

#define AAAAA { \
    blink_led (PWRLEDBUTT_LEDT_STANDBY); \
    this->next_state(PWRLEDBUTT_STATE_STANDBY); \
    StateMachine::Event ev(PWRLEDBUTT_EVT_ENTER_STANDBY); \
    this->add_event (ev); \
  }

void
PowerLedButton::process_event (StateMachine::Event &ev)
{
    TRACE3 ("PowerLedButton: %s(%d) on %s(%d)", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()), ev.get_type());
    switch (this->current_state()) {
    case PWRLEDBUTT_STATE_STANDBY:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ENTER_STANDBY:
            // stop any timer
            this->timer.stop();
            // start timer 1: the time stay in standby before enter to sleep mode
            //    the next timer 2 would be the time before call on_sleep
            TRACE0("Setup 1st timer for SLP1, tm=", (this->get_timeout_sleep()));
            this->timer.begin(get_timeout_sleep()/10)
               .repeat (10)
               .onTimer( [] ( int idx, int v, int up ) {
                       // Something to do when the timer goes off
                       if (v == 1) {
                           PowerLedButton *pthis;
                           pthis = (PowerLedButton *)idx;
                           StateMachine::Event ev(PWRLEDBUTT_EVT_TIMEOUT_SLEEP1);
                           pthis->add_event (ev);
                       }
                   }, (int)this)
               .onFinish( [] ( int idx, int v, int up ) {
                       // Something to do when the timer goes off
                       PowerLedButton *pthis;
                       pthis = (PowerLedButton *)idx;
                       TRACE0 ("ATM timeout SLP1");
                       StateMachine::Event ev(PWRLEDBUTT_EVT_TIMEOUT_SLEEP2);
                       pthis->add_event (ev);
                   }, (int)this)
               .start();
            break;
        case PWRLEDBUTT_EVT_TIMEOUT_SLEEP1:
            this->blink_led (PWRLEDBUTT_LEDT_OFF);
            break;
        case PWRLEDBUTT_EVT_TIMEOUT_SLEEP2:
            TRACE0 ("PowerLedButton: CB sleep");
            if (this->cb_sleep) {
                this->cb_sleep (this->userdata);
            }
            break;

        case PWRLEDBUTT_EVT_ONBEGIN:
            this->timer.stop();
            TRACE0 ("PowerLedButton: CB poweron");
            if (this->cb_poweron) {
                this->cb_poweron (this->userdata);
            }
            blink_led (PWRLEDBUTT_LEDT_WAITON);
            this->next_state(PWRLEDBUTT_STATE_BOOT_RELEASE);
            break;
        case PWRLEDBUTT_EVT_ONSIGOFF:
            SWITCH_STANDBY();
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            this->timer.stop();
            TRACE0 ("PowerLedButton: CB poweron");
            if (this->cb_poweron) {
                this->cb_poweron (this->userdata);
            }
            blink_led (PWRLEDBUTT_LEDT_ON);
            this->next_state(PWRLEDBUTT_STATE_ON);
            break;
        default:
            TRACE3 ("PowerLedButton: ST %s(%d) Unhandled : %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_BOOT_RELEASE:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONCLICK:
        case PWRLEDBUTT_EVT_ONLONG:
            this->next_state(PWRLEDBUTT_STATE_BOOT_WAIT);
            break;
        case PWRLEDBUTT_EVT_ONSIGOFF:
            SWITCH_STANDBY();
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            this->next_state(PWRLEDBUTT_STATE_ON);
            break;
        default:
            TRACE3 ("PowerLedButton: ST %s(%d) Unhandled : %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_BOOT_WAIT:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONSIGOFF:
        case PWRLEDBUTT_EVT_ONLONG:
            TRACE0 ("PowerLedButton: CB forced off");
            if (this->cb_forceoff) {
                this->cb_forceoff (this->userdata);
            }
            SWITCH_STANDBY();
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            this->next_state(PWRLEDBUTT_STATE_ON);
            break;
        default:
            TRACE3 ("PowerLedButton: ST %s(%d) Unhandled : %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_ON:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONSIGOFF:
        case PWRLEDBUTT_EVT_ONLONG:
            TRACE0 ("PowerLedButton: CB forced off");
            if (this->cb_forceoff) {
                this->cb_forceoff (this->userdata);
            }
            SWITCH_STANDBY();
            break;
        case PWRLEDBUTT_EVT_ONCLICK:
            TRACE0 ("PowerLedButton: CB shutdown");
            if (this->cb_shutdown) {
                this->cb_shutdown (this->userdata);
            }
            this->timer.begin(get_timeout_shutdown())
                       .onTimer( [] ( int idx, int v, int up ) {
                               // Something to do when the timer goes off
                               TRACE0 ("ATM timeout");
                               PowerLedButton *pthis;
                               pthis = (PowerLedButton *)idx;
                               StateMachine::Event ev(PWRLEDBUTT_EVT_TIMEOUT_SHUTDOWN);
                               pthis->add_event (ev);
                           }, (int)this)
                       .start();
            blink_led (PWRLEDBUTT_LEDT_WAITOFF);
            this->next_state(PWRLEDBUTT_STATE_SHUTDOWN);
            break;
        case PWRLEDBUTT_EVT_ONSIGRDY:
            blink_led (PWRLEDBUTT_LEDT_ON);
            //this->next_state(PWRLEDBUTT_STATE_ON);
            break;
        default:
            TRACE3 ("PowerLedButton: ST %s(%d) Unhandled : %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    case PWRLEDBUTT_STATE_SHUTDOWN:
        switch (ev.get_type()) {
        case PWRLEDBUTT_EVT_ONSIGOFF:
        case PWRLEDBUTT_EVT_TIMEOUT_SHUTDOWN:
        case PWRLEDBUTT_EVT_ONLONG:
            this->timer.stop();
            TRACE0 ("PowerLedButton: CB forced off");
            if (this->cb_forceoff) {
                this->cb_forceoff (this->userdata);
            }
            SWITCH_STANDBY();
            break;
        default:
            TRACE3 ("PowerLedButton: ST %s(%d) Unhandled : %s", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_EVT(ev.get_type()));
            break;
        }
        break;

    default:
        TRACE3 ("Error: unknown state: %s(%d)", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state());
        SWITCH_STANDBY();
        break;
    }
    TRACE3 ("StateMachine: ST %s(%d) --> %s(%d)", VAL2CSTR_PWRLEDBUTT_STATE(this->current_state()), this->current_state(), VAL2CSTR_PWRLEDBUTT_STATE(this->state_next), this->state_next);
}


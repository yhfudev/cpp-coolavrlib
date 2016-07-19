/**
 * @file    pwrledbutt.h
 * @brief   Power Button with LED
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */
/**
 * Usage:
 *   The Power Button class supports:
 *     1) de-bounce
 *     2) 1 click to power on or power off
 *     3) long press to force shutdown
 *   All of thess events can be obtained by callback functions.
 *
 *   Example:
 *     #define PORT_SWITCH   3
 *     #define PORT_LED_PWM 11
 *     void
 *     butt_on_poweron(void * userdata)
 *     {
 *         TRACE0 ("INFO: poweron pressed");
 *     }
 *     void
 *     butt_on_shutdown(void * userdata)
 *     {
 *         TRACE0 ("INFO: shutdown pressed");
 *     }
 *     void
 *     butt_on_force_off(void * userdata)
 *     {
 *         TRACE0 ("INFO: forced off pressed");
 *     }
 *     PowerLedButton2 butt;
 *     void setup(void) {
 *         Serial.begin(9600);
 *         delay(100);
 *         pinMode(PORT_LED_PWM, OUTPUT);
 *         pinMode(PORT_SWITCH, INPUT_PULLUP);
 *         butt.set_butt(PORT_SWITCH);
 *         butt.set_led(PORT_LED_PWM);
 *         butt.set_user_data(nullptr);
 *         butt.on_poweron (butt_on_poweron);
 *         butt.on_shutdown (butt_on_shutdown);
 *         butt.on_force_off (butt_on_force_off);
 *     }
 *     void loop(void) {
 *         butt.update();
 *     }
 */

#ifndef _POWER_LED_BUTTON2_H
#define _POWER_LED_BUTTON2_H

#include <Automaton.h>

#include "sysport.h"
#include "statemachine.h"

// int event( int id ); return if there's event that generate the event id
// void action( int id ); do the action by the id

#if DEBUG
#define PWRLEDBUTT_TIMEOUT_SHUTDOWN 5000
#define PWRLEDBUTT_TIMEOUT_SLEEP    5000

#else
#ifndef PWRLEDBUTT_TIMEOUT_SHUTDOWN
#define PWRLEDBUTT_TIMEOUT_SHUTDOWN     180000 /* 180000 -- 180 seconds and then force power off when not get a signal OFF */
#endif

#ifndef PWRLEDBUTT_TIMEOUT_SLEEP
#define PWRLEDBUTT_TIMEOUT_SLEEP     30000 /* 30000 -- 30 seconds to go to sleep when at standby mode */
#endif

#endif

class PowerLedButton : public StateMachine {
public:

    PowerLedButton();

    inline void set_led (uint8_t pwm_pin) { this->led.begin (pwm_pin); }
    inline void set_butt (uint8_t digital_pin) { this->butt.begin(digital_pin); }

    inline void set_user_data (void * userdata1) { this->userdata = userdata1; }
    inline void on_poweron ( void (*function)(void * userdata) ) { this->cb_poweron = function; }
    inline void on_shutdown ( void (*function)(void * userdata) ) { this->cb_shutdown = function; }
    inline void on_force_off ( void (*function)(void * userdata) ) { this->cb_forceoff = function; }
    inline void on_sleep ( void (*function)(void * userdata) ) { this->cb_sleep = function; }

    void setup (void); // prepare to ready

    // user called:
    void signal_off (void);   // told the class that host is off now
    void signal_ready (void); // told the class that host is on and ready

#define PWRLEDBUTT_LEDT_NONE    0
#define PWRLEDBUTT_LEDT_WAITON  1
#define PWRLEDBUTT_LEDT_WAITOFF 2
#define PWRLEDBUTT_LEDT_STANDBY 3
#define PWRLEDBUTT_LEDT_ON      4
#define PWRLEDBUTT_LEDT_OFF     5
    void blink_led (int type);

private:
    void * userdata;
    void (*cb_poweron)(void * userdata);
    void (*cb_shutdown)(void * userdata);
    void (*cb_forceoff)(void * userdata);
    void (*cb_sleep)(void * userdata); // called when at standby more than 30 seconds

    Atm_button butt;
    Atm_fade led;
    Atm_bit bit_butt;
    Atm_bit bit_fade;
    Atm_controller ctrl_1;
    Atm_controller ctrl_2;
    Atm_controller ctrl_3;
    Atm_timer timer;

    inline unsigned long get_timeout_shutdown() { return PWRLEDBUTT_TIMEOUT_SHUTDOWN; }
    inline unsigned long get_timeout_sleep() { return PWRLEDBUTT_TIMEOUT_SLEEP; }

    virtual void process_event (StateMachine::Event &ev); // process event, return the next state
    void enter_standby (void); // force to set the state to standby state
};

#endif // _POWER_LED_BUTTON2_H


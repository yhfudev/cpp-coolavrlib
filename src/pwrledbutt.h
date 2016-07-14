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
 *     PowerLedButton butt;
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

#ifndef _POWER_LED_BUTTON_H
#define _POWER_LED_BUTTON_H

#include "button.h"
#include "ledblink.h"

#ifndef PWRLEDBUTT_TIMEOUT_SHUTDOWN
#define PWRLEDBUTT_TIMEOUT_SHUTDOWN     180000 /* 180000 -- 180 seconds*/
#endif

#ifndef PWRLEDBUTT_LED_BRIGHT_ON
#define PWRLEDBUTT_LED_BRIGHT_ON     150
#endif

#ifndef PWRLEDBUTT_LED_BRIGHT_SB1
#define PWRLEDBUTT_LED_BRIGHT_SB1    0
#endif

#ifndef PWRLEDBUTT_LED_BRIGHT_SB2
#define PWRLEDBUTT_LED_BRIGHT_SB2    40
#endif

#ifndef PWRLEDBUTT_LED_TIME_FADE
#define PWRLEDBUTT_LED_TIME_FADE    1200
#endif

class PowerLedButton {
public:
    PowerLedButton();

    inline void set_led (uint8_t pwm_pin) { this->led_pwm.set_pin (pwm_pin); }
    inline void set_butt (uint8_t digital_pin) { this->butt.set_pin (digital_pin); }

    inline void set_user_data (void * userdata1) { this->userdata = userdata1; }
    inline void on_poweron ( void (*function)(void * userdata) ) { this->cb_poweron = function; }
    inline void on_shutdown ( void (*function)(void * userdata) ) { this->cb_shutdown = function; }
    inline void on_force_off ( void (*function)(void * userdata) ) { this->cb_forceoff = function; }

    // Update the LEDs and button status
    void update(void);

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

    unsigned long timer_prev; // the time last updated
    unsigned long timer_len;  // the timer length
    unsigned long timer_accu; // the accumulated time
    void cancle_timer ();
    int update_timer ();
    void start_timer (int time_ms);

    void update_other ();

    int led_type;
    LEDBlink led_pwm;
    void update_led ();

    #define PWRLEDBUTT_DIR_UP 1
    #define PWRLEDBUTT_DIR_DOWN -1
    int led_direction;

    Button butt;
    void update_butt ();

    inline unsigned long get_timeout_shutdown() { return PWRLEDBUTT_TIMEOUT_SHUTDOWN; }
    inline uint8_t get_led_bright_on() { return PWRLEDBUTT_LED_BRIGHT_ON; }
    inline uint8_t get_led_bright_sb1() { return PWRLEDBUTT_LED_BRIGHT_SB1; }
    inline uint8_t get_led_bright_sb2() { return PWRLEDBUTT_LED_BRIGHT_SB2; }
    inline unsigned long get_led_time_fade() { return PWRLEDBUTT_LED_TIME_FADE; }

    uint8_t process_event (Button::Event &ev); // process event, return the next state
    uint8_t current_state; // the current internal state

    friend void on_butt_begin (void * userdata);
    friend void on_butt_end (void * userdata);
    friend void on_butt_click (void * userdata, unsigned int times);
    friend void on_butt_long  (void * userdata);
};

#endif // _POWER_LED_BUTTON_H


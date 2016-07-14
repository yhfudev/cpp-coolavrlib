#include <system_configuration.h>
#include <StandardCplusplus.h>
#include <utility.h>
#include <unwind-cxx.h>

/**
 * @file    pwrledbuttexample.ino
 * @brief   Example of Power Button with LED
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */
#include "sysport.h"
#include "pwrledbutt.h"

// https://github.com/n0m1/Sleep_n0m1.git
#include <Sleep_n0m1.h>
Sleep sleep;

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
#undef TRACE1
#define TRACE1(...)
#undef TRACE2
#define TRACE2(...)
//#undef TRACE3
//#define TRACE3(...)
#endif

#ifdef __AVR_ATtiny85__
#define PORT_SW_ONOFF  5
#define PORT_PWR_CTRL  4
#define PORT_LED_PWM   3
#else
#define PORT_SW_ONOFF  3
#define PORT_PWR_CTRL  2//5
#define PORT_LED_PWM   6
#endif

#define BLINK_TIME 200

PowerLedButton butt;
LEDBlink led_nopwm;

bool is_power_off = true;

#define TIME_SLEEP 10000
unsigned long tm_last_sleep = 0;
void
ledkey_updates (void)
{
    butt.update();
    led_nopwm.update();
    if (is_power_off) {
        if (! led_nopwm.is_busy()) {
            unsigned long now = millis ();
            if (now - tm_last_sleep > TIME_SLEEP) {
                tm_last_sleep = now;
                TRACE3 ("INFO: DEEP SLEEP");
#if 1
                butt.blink_led (PWRLEDBUTT_LEDT_OFF);
                delay (200); //delay to allow serial to fully print before sleep
                sleep.pwrDownMode(); //set sleep mode
                //Sleep till interrupt pin equals a particular state.
                //In this case "low" is state 0.
                sleep.sleepPinInterrupt(PORT_SW_ONOFF, LOW); //(interrupt Pin Number, interrupt State)
#endif
            }
        }
    }
}

void
butt_on_poweron(void * userdata)
{
    TRACE0 ("INFO: poweron pressed");
    led_nopwm.start_blink(BLINK_TIME, 3);
    is_power_off = false;
}

void
butt_on_shutdown(void * userdata)
{
    TRACE0 ("INFO: shutdown pressed");
    led_nopwm.start_blink(BLINK_TIME, 2);
}

void
butt_on_force_off(void * userdata)
{
    TRACE0 ("INFO: forced off pressed");
    led_nopwm.start_blink(BLINK_TIME * 4, 3);
    is_power_off = true;
}

uint8_t g_state_sig = 0;
void
update_signal ()
{
    uint8_t state_butt;
    state_butt = digitalRead(PORT_PWR_CTRL);
    if (g_state_sig != state_butt) {
        g_state_sig = state_butt;
        if (state_butt != LOW) {
            TRACE3 ("INFO: signal ready");
            butt.signal_ready();
        } else {
            TRACE3 ("INFO: signal off");
            butt.signal_off();
        }
    }
}

void
setup(void)
{
#if DEBUG
    Serial.begin(9600);
    delay(100);
#endif
    pinMode(PORT_PWR_CTRL, INPUT_PULLUP);

    pinMode(PORT_LED_PWM, OUTPUT);
    pinMode(PORT_SW_ONOFF, INPUT_PULLUP);
    butt.set_butt(PORT_SW_ONOFF);
    butt.set_led(PORT_LED_PWM);
    butt.set_user_data(nullptr);
    butt.on_poweron (butt_on_poweron);
    butt.on_shutdown (butt_on_shutdown);
    butt.on_force_off (butt_on_force_off);
    butt.blink_led (PWRLEDBUTT_LEDT_STANDBY);

    pinMode(LED_BUILTIN, OUTPUT);
    led_nopwm.set_pin(LED_BUILTIN); // digital pin 13.
}

void
loop(void)
{
    ledkey_updates();
    update_signal ();
}


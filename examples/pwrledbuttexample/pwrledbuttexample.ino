/**
 * @file    pwrledbuttexample.ino
 * @brief   Example of Power Button with LED
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */
#include "sysport.h"
#include "ledblink.h"
#include "pwrledbutt.h"

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


#if defined(__AVR__)
#include <avr/sleep.h>

void sleep_on_wakeup()
{
    // execute code here after wake-up before returning to the loop() function
    // timers and code using timers (serial.print and more...) will not work here.
    // we don't really need to execute any special functions here, since we
    // just want the thing to wake up
}

void sleep_setup()
{
    //pinMode(wakePin, INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(2), sleep_on_wakeup, LOW); // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW
    attachInterrupt(digitalPinToInterrupt(PORT_SW_ONOFF), sleep_on_wakeup, CHANGE);
}

void sleep_now()
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here
    sleep_enable();          // enables the sleep bit in the mcucr register
    //attachInterrupt(digitalPinToInterrupt(2), sleep_on_wakeup, LOW); // use interrupt 0 (pin 2) and run function
    attachInterrupt(digitalPinToInterrupt(PORT_SW_ONOFF), sleep_on_wakeup, CHANGE);
    sleep_mode();            // here the device is actually put to sleep!!
    // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
    sleep_disable();         // first thing after waking from sleep: disable sleep...
    detachInterrupt(digitalPinToInterrupt(PORT_SW_ONOFF));      // disables interrupt 0 on pin 2 so the wakeUpNow code will not be executed during normal running time.
}
#else
#define sleep_setup() __NOT_IMPMENTED_sleep_setup
#define sleep_now() __NOT_IMPMENTED_sleep_now
#endif // AVR

PowerLedButton butt;
LEDBlink led_nopwm;

void
ledkey_updates (void)
{
    led_nopwm.update();
}

void
butt_on_poweron(void * userdata)
{
    TRACE0 ("INFO: poweron pressed");
    led_nopwm.start_blink(BLINK_TIME, 6);
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
}

void
butt_on_sleep(void * userdata)
{
    TRACE0 ("INFO: forced sleep");
    delay(200); // wait for debug message send to Serial
    sleep_now();
}

uint8_t g_state_sig = LOW;
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
#if DEBUG && defined(ARDUINO)
    Serial.begin(9600);
    // Wait for USB Serial.
    //while (!Serial) {}

    // Read any input
    delay(200);
    while (Serial.read() >= 0) {}
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
    butt.on_sleep (butt_on_sleep);
    butt.setup();

    pinMode(LED_BUILTIN, OUTPUT);
    led_nopwm.set_pin(LED_BUILTIN); // digital pin 13.

    sleep_setup();
}

void
loop(void)
{
    ledkey_updates();
    update_signal ();
    automaton.run();
}

#if ! defined(ARDUINO)
int
main(void)
{
    setup();
    while (1) {
        loop();
        delay(500);
    }
    return 0;
}
#endif


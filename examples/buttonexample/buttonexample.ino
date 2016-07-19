
#include "sysport.h"
#include "button.h"

#ifndef TRACE
#define TRACE(...)
#endif

// 0 -- verbose, 1 -- info, 2 -- warning, 3 -- error
#define TRACE0 TRACE
#define TRACE1 TRACE
#define TRACE2 TRACE
#define TRACE3 TRACE

#define PORT_SWITCH   3
#define PORT_LED_PWM   6

void
butt_on_click(void * userdata, unsigned int times)
{
    int pin = (intptr_t)(userdata);
    TRACE0 ("INFO: switch pin %d pressed %d times", pin, times);
}

void
butt_on_longpress(void * userdata)
{
    int pin = (intptr_t)(userdata);
    TRACE0 ("INFO: switch pin %d long pressed", pin);
}

void
butt_on_vlongpress(void * userdata)
{
    int pin = (intptr_t)(userdata);
    TRACE0 ("INFO: switch pin %d very long pressed", pin);
}

void
butt_on_begin(void *userdata)
{
   digitalWrite (LED_BUILTIN, HIGH);
   digitalWrite (PORT_LED_PWM, HIGH);
}

void
butt_on_end(void *userdata)
{
   digitalWrite (LED_BUILTIN, LOW);
   digitalWrite (PORT_LED_PWM, LOW);
}

Button butt(false);

void
setup(void)
{
#if DEBUG && defined(ARDUINO)
    Serial.begin(9600);
    // Wait for USB Serial.
    while (!Serial) {}

    // Read any input
    delay(200);
    while (Serial.read() >= 0) {}
#endif

    pinMode (PORT_SWITCH, INPUT_PULLUP);
    butt.set_pin (PORT_SWITCH);
    butt.set_user_data ((void *)PORT_SWITCH);
    butt.on_click (butt_on_click);
    butt.on_long_press (butt_on_longpress);
    butt.on_vlong_press (butt_on_vlongpress);
    butt.on_start (butt_on_begin);
    butt.on_end (butt_on_end);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PORT_LED_PWM, OUTPUT);
}

void
loop(void)
{
    butt.update();
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

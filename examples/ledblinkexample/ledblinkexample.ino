
#include "sysport.h"
#include "ledblink.h"

#ifndef TRACE
#define TRACE(...)
#endif

// 0 -- verbose, 1 -- info, 2 -- warning, 3 -- error
#define TRACE0 TRACE
#define TRACE1 TRACE
#define TRACE2 TRACE
#define TRACE3 TRACE

#define PORT_LED_PWM 11

//////////////////////////////////////////////////////////////////////
// LEDs
#define DIR_UP 1
#define DIR_DOWN -1
int g_led_direction = DIR_UP;

#define LED_V_FADE1 0
#define LED_V_FADE2 20

#define FADE_TIME 1000
#define BLINK_TIME 200

#if 0 // DEBUG
#undef LED_V_FADE2
#define LED_V_FADE2 20
#undef FADE_TIME
#define FADE_TIME 200
#endif

void
fading_up_down_update (LEDBlink *plf, int * p_direction)
{
    if (NULL == plf) {
        return;
    }
    plf->update();
    if (! plf->is_blinking()) {
        if (NULL == p_direction) {
            return;
        }
        // LED no longer fading, switch direction
        if (! plf->is_busy()) {
            if (*p_direction == DIR_UP) {
                // Fade down
                *p_direction = DIR_DOWN;
                plf->start_fade(FADE_TIME, LED_V_FADE1, LED_V_FADE2);
            } else {
                // Fade up
                *p_direction = DIR_UP;
                plf->start_fade(FADE_TIME, LED_V_FADE2, LED_V_FADE1);
            }
        }
    }
}

LEDBlink led_pwm;
LEDBlink led_nopwm;

void
setup(void)
{
    pinMode(PORT_LED_PWM, OUTPUT);
    led_pwm.set_pin(PORT_LED_PWM);
    led_pwm.start_fade(FADE_TIME, LED_V_FADE1, LED_V_FADE2);
    //led_pwm.set_value (250);

    pinMode(LED_BUILTIN, OUTPUT);
    led_nopwm.set_pin(LED_BUILTIN); // digital pin 13.
    led_nopwm.start_blink(500, 200000);
}

void
loop(void)
{
    fading_up_down_update(&led_pwm, &g_led_direction);
    led_nopwm.update();
}


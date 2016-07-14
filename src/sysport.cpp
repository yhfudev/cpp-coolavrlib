/**
 * @file    sysport.h
 * @brief   port the arduino programs to PC platform for debug
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */

#include "sysport.h"

#if ! defined(ARDUINO)

// test time 60000 -- 60 seconds
#define TIME_TEST 60000

#define FADE_TIME 2000

#define TRACE_NOTM(fmt, ...) fprintf (stdout, "TIME [%s()] " fmt " {ln:%d, fn:" __FILE__ "}\n", __func__, ##__VA_ARGS__, __LINE__)

unsigned long
millis0(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long time_in_mill = ((tv.tv_sec) % (TIME_TEST * 2)) * 1000 + (tv.tv_usec) / 1000;
    return time_in_mill;
}

static unsigned long m_tm_mill_pre = 1500;
unsigned long
millis(void)
{
    m_tm_mill_pre += (3 + (rand() % 4));
    if (m_tm_mill_pre > TIME_TEST) {
        // automatically quit testing after 20 seconds
        TRACE_NOTM ("Time ended, test end");
        exit (1);
    }
    return m_tm_mill_pre;
}

#define PORT_LED_PWM 11  // the led for PWM(fade) control

#define PORT_MSCONF      12 // config the device as master or slave
#define PORT_PWR_CTRL    5
#define PORT_SW_ONOFF    3
#define PORT_ADC_BATTPWR A0
#define PORT_ADC_EXTPWR  A1

#define LED_BUILTIN 13


#define BUTSW_TIMEOUT_DBOUNCE   30

void
analogWrite (uint8_t pin, int val)
{
    assert (LED_BUILTIN != pin);
    return;
}

static unsigned long m_tm_pre = 0;
static int m_digi_val = 0;
int
digitalRead(uint8_t pin)
{
    if (pin == PORT_MSCONF) {
        return 0;
    }
    if (pin != PORT_SW_ONOFF) {
        return 0;
    }
    unsigned long now = millis();
    if (m_digi_val) {
        if (m_tm_pre + BUTSW_TIMEOUT_DBOUNCE * 5 > now) {
            return 1;
        }
    }
    m_digi_val = 0;
    // return 1 very seconds
    int randv = rand() % 100;
    if (m_tm_pre + (TIME_TEST / 3 + randv) < now) {
        m_tm_pre = now;
        m_digi_val = 1;
        return 1;
    }
    return 0;
}

#endif



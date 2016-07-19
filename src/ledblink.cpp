/**
 * @file    ledblink.h
 * @brief   LED Blink Lib for Arduino
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */

#include "sysport.h"
#include "ledblink.h"

/**
TODO:
timer and callback to change the LED directly.
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
#undef TRACE1
//#undef TRACE2
//#undef TRACE3
#define TRACE0(...)
#define TRACE1(...)
//#define TRACE2(...)
//#define TRACE3(...)
#endif

#if 0
static const PROGMEM uint8_t etable[256] = {
   0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,
   3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,
   4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,
   6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  8,  8,  8,
   8,  8,  8,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 11, 11, 11,
  11, 12, 12, 12, 12, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 16,
  16, 16, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
  23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28, 29, 30, 30, 31, 32,
  32, 33, 34, 35, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 52, 53, 55, 56, 57, 58, 59, 61, 62, 63,
  65, 66, 68, 69, 71, 72, 74, 76, 77, 79, 81, 82, 84, 86, 88, 90,
  92, 94, 96, 98,100,102,105,107,109,112,114,117,119,122,124,127,
 130,133,136,139,142,145,148,151,155,158,162,165,169,172,176,180,
 184,188,192,196,201,205,210,214,219,224,229,234,239,244,250,255,
};

#define READ_ETAB(i) pgm_read_byte(&etable[i])
#else
#define READ_ETAB(i) (i)
#endif

#if DEBUG
void
LEDBlink::check_integrate (void)
{
    if (this->color_first == this->color_last) {
        if (this->times_onoff > 0) {
            return;
        }
        if (tm_accum < tm_length) {
            TRACE3("Err in class");
            assert (0);
        }
    }
}
#define LEDB_CHECK_INTEGRATE() this->check_integrate()
#else
#define LEDB_CHECK_INTEGRATE()
#endif

LEDBlink::LEDBlink()
{
    this->pin = 0;

    this->interval = 0;
    this->last_step_time = 0;
    this->tm_accum = 1;
    this->tm_length = 1;

    this->times_onoff = 0;

    this->color_first = 0;
    this->color_last = 0;
    this->color_pre = 0;

    LEDB_CHECK_INTEGRATE();
}

bool
is_in_range_8b (uint8_t a, uint8_t b, uint8_t v)
{
    if (a < b) {
        return (a <= v && v <= b);
    } else {
        return (b <= v && v <= a);
    }
}

void
LEDBlink::set_value(int value)
{
    LEDB_CHECK_INTEGRATE();

    if (! this->pin) {
        TRACE3 ("LEDBlink setvalue failed, pin not set!");
        return;
    }
    if (is_fade()) {
        uint8_t color = (uint8_t)constrain(value, 0, 255);
        if (color == this->color_pre) {
            TRACE3 ("LEDBlink ignore the same color as previous: %d!", (int)color);
            return;
        }
        TRACE0 ("LEDBlink set pin(%d)=%d by analogWrite", (int)this->pin, (int)value);
        analogWrite(pin, READ_ETAB(color));
        this->color_pre = color;
        if (! is_in_range_8b (this->color_first, this->color_last, value)) {
            this->tm_accum = this->tm_length;
            LEDB_CHECK_INTEGRATE();
        }
    } else {
        TRACE0 ("LEDBlink set pin(%d)=%d by digitalWrite", (int)this->pin, (int)value);
        digitalWrite (this->pin, (value?HIGH:LOW));
    }
    LEDB_CHECK_INTEGRATE();
}

bool
LEDBlink::is_busy()
{
    LEDB_CHECK_INTEGRATE();

    if (! this->pin) {
        TRACE3 ("LEDBlink is_busy failed, pin not set!");
        return false;
    }
    if (is_blinking()) {
        return true;
    }
    if (is_fade()) {
        return true;
    }
    return false;
}

void
LEDBlink::stop()
{
    LEDB_CHECK_INTEGRATE();

    if (is_fade() || is_fade_prev()) {
        this->set_value (this->color_last);
        this->color_first = this->color_last;
    } else {
        this->set_value (LOW);
        this->color_first = 0;
        this->color_last = 0;
    }
    this->times_onoff = 0;
    this->tm_accum = this->tm_length;
}

void
LEDBlink::start_blink(unsigned long time_ms, unsigned int times_onoff)
{
    LEDB_CHECK_INTEGRATE();

    // No pin defined
    if (! this->pin) {
        TRACE3 ("LEDBlink start failed, pin not set!");
        return;
    }

    this->color_first = 0;
    this->color_last = 0;
    this->tm_length = time_ms;
    this->tm_accum = 0;
    this->last_step_time = millis();

    if (time_ms <= LEDBLINK_MIN_INTERVAL) {
        set_value (LOW);
        TRACE3 ("LEDBlink time too short!");
        LEDB_CHECK_INTEGRATE();
        return;
    }

    this->times_onoff = times_onoff;
    if (this->times_onoff < 1) {
        this->times_onoff = 1;
    }

    // Figure out what the interval should be so that we're chaning the color by at least 1 each cycle
    // (minimum interval is LEDBLINK_MIN_INTERVAL)
    // the LED will be off in 1/3 time and 2/3 on
    this->interval = round((float)this->tm_length / 3);
    if (this->interval < LEDBLINK_MIN_INTERVAL) {
        this->interval = LEDBLINK_MIN_INTERVAL;
    }
    if (this->tm_length < this->interval * 3) {
        this->tm_length = this->interval * 3;
    }
    LEDB_CHECK_INTEGRATE();
}

void
LEDBlink::start_fade(unsigned long time_ms, uint8_t pwm_first, uint8_t pwm_last)
{
    LEDB_CHECK_INTEGRATE();

    TRACE0("start fade: time=%d pwm_first=%d pwm_last=%d", time_ms, pwm_first, pwm_last);
    //this->stop();

    // No pin defined
    if (! this->pin) {
        TRACE3 ("LEDBlink start failed, pin not set!");
        return;
    }

    if (time_ms <= LEDBLINK_MIN_INTERVAL) {
        TRACE3 ("LEDBlink time too short!");
        return;
    }

    this->times_onoff = 0;
    this->tm_length = time_ms;
    this->tm_accum = 0;
    this->last_step_time = millis();

    this->color_first = (uint8_t)constrain(pwm_first, 0, 255);
    this->color_last  = (uint8_t)constrain(pwm_last, 0, 255);

    // Figure out what the interval should be so that we're chaning the color by at least 1 each cycle
    // (minimum interval is LEDBLINK_MIN_INTERVAL)
    float color_diff = abs(this->color_last - this->color_first);
    this->interval = round((float)this->tm_length / color_diff);
    if (this->interval < LEDBLINK_MIN_INTERVAL) {
        this->interval = LEDBLINK_MIN_INTERVAL;
    }

    LEDB_CHECK_INTEGRATE();
}


bool
LEDBlink::advance_ms(unsigned int time_diff)
{
    this->tm_accum += time_diff;
    if (this->tm_accum >= this->tm_length) {
        if (is_fade()) {
            this->set_value (this->color_last);
            this->color_first = this->color_last;
            this->tm_accum = this->tm_length;
            //LEDB_CHECK_INTEGRATE();
        } else {
            this->tm_accum = 0;
            //LEDB_CHECK_INTEGRATE();
            set_value(LOW);
            this->times_onoff --;
            if (this->times_onoff < 1) {
                this->tm_accum = this->tm_length;
            }
            //LEDB_CHECK_INTEGRATE();
        }
    }
    if (! is_busy()) {
        TRACE0 ("LEDBlink task was finished, Skip update!");
        //LEDB_CHECK_INTEGRATE();
        return false;
    }

    if (is_fade()) {
        // fade
        uint8_t color = this->color_first;
        unsigned long color_diff = abs(this->color_last - this->color_first);
        uint8_t incval = (uint8_t)(this->tm_accum * color_diff / this->tm_length);
        if (color < this->color_last) {
            color += incval;
            TRACE0 ("LEDBlink fade color up pin:%d +%d --> %d", (int)this->pin, (int)incval, (int)color);
        } else {
            color -= incval;
            TRACE0 ("LEDBlink fade color down pin:%d -%d --> %d", (int)this->pin, (int)incval, (int)color);
        }
        set_value(color);
        //LEDB_CHECK_INTEGRATE();
    } else {
        if (0 == ((this->tm_accum / this->interval) % (3) )) {
            // off
            TRACE0 ("LEDBlink OFF");
            set_value(LOW);
            //LEDB_CHECK_INTEGRATE();
        } else {
            // on
            TRACE0 ("LEDBlink ON");
            set_value(HIGH);
            //LEDB_CHECK_INTEGRATE();
        }
    }
    return true;
}

// update the led by checking the time.
bool
LEDBlink::update()
{
    LEDB_CHECK_INTEGRATE();

    // No pin defined
    if (! this->pin) {
        TRACE3 ("LEDBlink is_busy failed, pin not set!");
        return false;
    }

    // No blink
    if (! is_busy()) {
        //TRACE2 ("LEDBlink task was finished, Skip update!");
        LEDB_CHECK_INTEGRATE();
        return false;
    }

    unsigned long now = millis();
    unsigned int time_diff = now;
    if (now >= this->last_step_time) {
        time_diff = now - this->last_step_time;
    }

    // Interval hasn't passed yet
    if (time_diff < this->interval) {
        LEDB_CHECK_INTEGRATE();
        return true;
    }

    this->advance_ms (time_diff);

    this->last_step_time = now;

    LEDB_CHECK_INTEGRATE();
    return true;
}


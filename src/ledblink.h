/**
 * @file    ledblink.h
 * @brief   LED Blink Lib for Arduino
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */
/**
 * Usage:
 *   The LEDBlink class supports:
 *     1) blink
 *     2) fade
 *
 *   Example:
 *     #define PORT_LED_PWM 11
 *     LEDBlink led_pwm;
 *     LEDBlink led_nopwm;
 *     void setup(void) {
 *         pinMode(PORT_LED_PWM, OUTPUT);
 *         led_pwm = LEDBlink(PORT_LED_PWM);
 *         led_pwm.start_fade(2000, 0, 127);
 *         //led_pwm.set_value (250);
 *
 *         pinMode(LED_BUILTIN, OUTPUT);
 *         led_nopwm = LEDBlink(LED_BUILTIN); // digital pin 13.
 *         led_nopwm.start_blink(500, 200000);
 *     }
 *     void loop(void) {
 *         led_pwm.update();
 *         led_nopwm.update();
 *     }
 */

#ifndef _LED_BLINK_H
#define _LED_BLINK_H

class LEDBlink {
public:
    LEDBlink ();

    // Set the digital pin that the LED is connected to
    inline void set_pin(uint8_t pwm_pin) { this->pin = pwm_pin; }
    inline uint8_t get_pin(void) { return pin; }

    // Blink an LED over a duration of time time_ms(milliseconds) with times_onoff times.
    void start_blink(unsigned long time_ms, unsigned int times_onoff);
    // Stop the current work where it's at
    void stop();
    // fade an LED with a range of PWM value over a duration of milliseconds
    void start_fade(unsigned long time_ms, uint8_t pwm_first, uint8_t pwm_last);

    // Returns TRUE if there is an active blinking process
    bool is_busy();

    // Update the LEDs along the blinking
    // Returns TRUE if a blink is still in process
    bool update();

    // Returns how much of the blink is complete in a percentage between 0 - 100
    inline uint8_t get_progress() { return this->tm_accum * 100 / this->tm_length; }

    // // Set an LED to an absolute PWM value or status
    void set_value(int value);

    inline bool is_blinking () { if (this->times_onoff > 0) { return true; } return false; }
    inline bool is_fade () { if ((this->color_first == this->color_last)) { return false; } return true; }

#if DEBUG
    void check_integrate (void);
#endif

private:
    inline bool is_fade_prev () { if (this->color_last > 0) return true; return false; }

    uint8_t pin;
    unsigned long last_step_time;
    unsigned long interval;
    unsigned long tm_accum;
    unsigned long tm_length;

    unsigned int times_onoff; // for blinking
    uint8_t color_first;      // for fading
    uint8_t color_last;

    uint8_t color_pre; // last color set
};

// The minimum time (milliseconds) the program will wait between LED adjustments
// adjust this to modify performance.
#define LEDBLINK_MIN_INTERVAL 20

#endif // _LED_BLINK_H


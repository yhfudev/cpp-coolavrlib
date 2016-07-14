/**
 * @file    button.h
 * @brief   Button Click Lib for Arduino, supports 1 click, double clicks, long press, very long press
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */
/**
 * Usage:
 *   The Button class supports:
 *     1) de-bounce
 *     2) 1 click, double clicks, and multiple clicks
 *     3) long press
 *     4) very long press
 *     5) the begin and end of the click events
 *   All of thess events can be obtained by callback functions.
 *   The double clicks and multiple clicks can be enabled at initialization.
 *
 *   Example:
 *     #define PORT_SWITCH 3
 *     void
 *     butt_on_click(void * userdata, unsigned int times)
 *     {
 *         int pin = (int)(userdata);
 *         TRACE0 ("INFO: switch pin %d pressed %d times", pin, times);
 *     }
 *     void
 *     butt_on_longpress(void * userdata)
 *     {
 *         int pin = (int)(userdata);
 *         TRACE0 ("INFO: switch pin %d long pressed", pin);
 *     }
 *     void
 *     butt_on_vlongpress(void * userdata)
 *     {
 *         int pin = (int)(userdata);
 *         TRACE0 ("INFO: switch pin %d very long pressed", pin);
 *     }
 *     Button butt(false);
 *     void setup(void) {
 *         pinMode(PORT_SWITCH, INPUT_PULLUP);
 *         butt.set_pin(PORT_SWITCH);
 *         butt.set_user_data (PORT_SWITCH);
 *         butt.on_click (butt_on_click);
 *         butt.on_long_press (butt_on_longpress);
 *         butt.on_vlong_press (butt_on_vlongpress);
 *     }
 *     void loop(void) {
 *         butt.update();
 *     }
 */

#ifndef _BUTTON_SW_PUSH_H
#define _BUTTON_SW_PUSH_H 1

#ifndef BUTSW_TIMEOUT_DBOUNCE
// debounce 30ms
#define BUTSW_TIMEOUT_DBOUNCE   30
#endif
#ifndef BUTSW_TIMEOUT_LONG
#define BUTSW_TIMEOUT_LONG    1200
#endif
#ifndef BUTSW_TIMEOUT_VLONG
#define BUTSW_TIMEOUT_VLONG   3000
#endif
#ifndef BUTSW_TIMEOUT_2CLICK
#define BUTSW_TIMEOUT_2CLICK   250 // the time between two clicks to merge
#endif

// the returned button state types
#define BUTSW_TYPE_NONE        0
#define BUTSW_TYPE_1CLICK      1
#define BUTSW_TYPE_2CLICK      2
#define BUTSW_TYPE_LONGPRESS   3
#define BUTSW_TYPE_VLONGPRESS  4

class Button {
public:
    Button (bool multiple_click = false);
    class Event {
    public:
        Event (): type(0) {}
        Event (int type1): type(type1) {}
        inline uint8_t get_type() { return this->type; }
        inline void set_type(uint8_t ty) { this->type = ty; }
    private:
        uint8_t type;
    };

    inline uint8_t get_pin(void) { return this->pin; }
    void set_pin(uint8_t digital_pin);
    void set_pin(uint8_t digital_pin, uint8_t preessed_state); // pressed_state: what's the state of the input, HIGH or LOW, when button pressed

    // Update the LEDs along the blinking
    // Returns TRUE if a blink is still in process
    bool update();

    uint8_t get_key_type (void); // return the current key type.

    inline void set_user_data (void * userdata1) { this->userdata = userdata1; }
    inline void on_click ( void (*function)(void * userdata, unsigned int) ) { this->OnClick = function; }
    inline void on_start ( void (*function)(void * userdata) ) { this->OnStart = function; }
    inline void on_end ( void (*function)(void * userdata) ) { this->OnEnd = function; }
    inline void on_long_press ( void (*function)(void * userdata) ) { this->OnLongPress = function; }
    inline void on_vlong_press ( void (*function)(void * userdata) ) { this->OnVLongPress = function; }

private:
    inline unsigned long get_timeout_dbounce() { return BUTSW_TIMEOUT_DBOUNCE; }
    inline unsigned long get_timeout_long()    { return BUTSW_TIMEOUT_LONG; }
    inline unsigned long get_timeout_vlong()   { return BUTSW_TIMEOUT_VLONG; }
    inline unsigned long get_timeout_2click()  { return BUTSW_TIMEOUT_2CLICK; }

    void update_pin(uint8_t pin_state);
    void update_other();

    unsigned long longTime;
    unsigned long shortTime;
    unsigned int lastButtonStatus;

    uint8_t pin;
    bool multiple_click; // if the module signal multiple click as one event
    bool button_hold; // if the button pressed and hold?
    uint8_t process_event (Button::Event &ev); // process event, return the next state
    uint8_t current_state; // the current internal state

    uint8_t pressed_state; // what's the state of the input, HIGH or LOW, when button pressed
    bool is_pressed (uint8_t cur_state);
    unsigned int clicks; // the adjacent clicks (in BUTSW_TIMEOUT_2CLICK) 

    unsigned long timer_prev; // the time last updated
    unsigned long timer_len;  // the timer length
    unsigned long timer_accu; // the accumulated time
    void cancle_timer ();
    int update_timer ();
    void start_timer (int time_ms);

    void * userdata;
    void (*OnClick)(void * userdata, unsigned int times);
    void (*OnLongPress)(void * userdata);
    void (*OnVLongPress)(void * userdata);
    void (*OnStart)(void * userdata); // start of press
    void (*OnEnd)(void * userdata); // release of key
};

#endif // _BUTTON_SW_PUSH_H


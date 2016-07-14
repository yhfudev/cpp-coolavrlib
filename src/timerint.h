/**
 * @file    timerint.h
 * @brief   Interruption based timer for Arduino
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-14
 * @copyright GPL
 */

#ifndef _TIMER_INTERRUPTION_ARDUINO_H
#define _TIMER_INTERRUPTION_ARDUINO_H

class TimerInt {
public:
    TimerInt ();

    // start a timer and return the timer id
    int start (unsigned long time_ms, void * userdata, void (*function)(void * userdata));
    int cancel (int id);

    class Item {
    public:
        //Item();
        int id;
        unsigned long time_left;
        void * userdata;
        void (* cb_expired)(void * userdata);
    };
private:
    std::vector<TimerInt::Item> lst;
};

//inline bool operator==(const X& lhs, const X& rhs){ /* do actual comparison */ }
//inline bool operator!=(const X& lhs, const X& rhs){ return !(lhs == rhs); }
//inline bool operator< (const X& lhs, const X& rhs){ /* do actual comparison */ }
//inline bool operator> (const X& lhs, const X& rhs){ return rhs < lhs; }
//inline bool operator<=(const X& lhs, const X& rhs){ return !(lhs > rhs); }
//inline bool operator>=(const X& lhs, const X& rhs){ return !(lhs < rhs); }
inline bool operator< (const TimerInt::Item & lhs, const TimerInt::Item & rhs)
{
    return (lhs.time_left < rhs.time_left);
}

inline bool operator== (const TimerInt::Item & lhs, const TimerInt::Item & rhs)
{
    return (lhs.time_left == rhs.time_left);
}

#endif // _TIMER_INTERRUPTION_ARDUINO_H


/**
 * @file    timerint.cpp
 * @brief   Interruption based timer for Arduino
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-14
 * @copyright GPL
 */

// https://github.com/andysworkshop/avr-stl.git
//#include <avr_config.h> // compile error
//https://github.com/vancegroup/stlport-avr.git
//#include <stlport.h> // compile error

// uclibc
// https://github.com/maniacbug/StandardCplusplus.git

#include <algorithm>    // std::make_heap, std::pop_heap, std::push_heap, std::sort_heap
#include <vector>       // std::vector

#include "timerint.h"

TimerInt::TimerInt()
{
}

int
TimerInt::start (unsigned long time_ms, void * userdata, void (*function)(void * userdata))
{
    Item item_new;
    item_new.userdata = userdata;
    item_new.cb_expired = function;
    this->lst.push_back (item_new);
    std::push_heap (this->lst.begin(), this->lst.end());
    return -1;
}

int
TimerInt::cancel (int id)
{
    return -1;
}



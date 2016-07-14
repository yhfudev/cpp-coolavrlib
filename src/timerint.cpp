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


#if 0
struct TestVector {

  static void RunTest() {

    std::ohserialstream serial(Serial);
    std::vector<int> vec;
    std::vector<int>::const_iterator it;
    int i;

    vec.reserve(50);
    for(i=0;i<50;i++)
      vec.push_back(i);

    for(it=vec.begin();it!=vec.end();it++)
      serial << *it << std::endl;
  }

};

#endif

struct TestHeap {
    static void RunTest()
    {
        int myints[] = {10,20,30,5,15};
        std::vector<int> v(myints,myints+5);

        std::make_heap (v.begin(),v.end());
        //serial << "initial max heap   : " << v.front() << '\n';

        std::pop_heap (v.begin(),v.end()); v.pop_back();
        //serial << "max heap after pop : " << v.front() << '\n';

        v.push_back(99); std::push_heap (v.begin(),v.end());
        //serial << "max heap after push: " << v.front() << '\n';

        std::sort_heap (v.begin(),v.end());

        //serial << "final sorted range :";
        for (unsigned i=0; i<v.size(); i++) {
          //serial << ' ' << v[i];
          ;
        }

        //serial << '\n';

        //return 0;
    }
};

#if 1
void RunTest2() {

    std::vector<int> vec;
    std::vector<int>::const_iterator it;
    int i;

    vec.reserve(50);
    for(i=0;i<50;i++)
      vec.push_back(i);

    for(it=vec.begin();it!=vec.end();it++)
      //serial << *it << std::endl;
      ;
}
#endif




/**
 * @file    stlexample.cpp
 * @brief   Test STL library
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-14
 * @copyright GPL
 */

#if defined(ARDUINO)
// https://github.com/andysworkshop/avr-stl.git
//#include <avr_config.h> // compile error
//https://github.com/vancegroup/stlport-avr.git
//#include <stlport.h> // compile error

// uclibc
// https://github.com/maniacbug/StandardCplusplus.git
#include <StandardCplusplus.h>
#endif

#include <algorithm>    // std::make_heap, std::pop_heap, std::push_heap, std::sort_heap
#include <vector>       // std::vector

#include "sysport.h"

struct TestVector {
    static void RunTest() {
        //std::ohserialstream serial(Serial);
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
};

struct TestHeap {
    static void RunTest()
    {
        //std::ohserialstream serial(Serial);
        int myints[] = {10,20,30,5,15};
        std::vector<int> v(myints,myints+5);

        std::make_heap (v.begin(),v.end());
        //serial << "initial max heap   : " << v.front() << '\n';
        TRACE ("initial max heap : %d", v.front());

        std::pop_heap (v.begin(),v.end()); v.pop_back();
        //serial << "max heap after pop : " << v.front() << '\n';
        TRACE ("max heap after pop : %d", v.front());

        v.push_back(99); std::push_heap (v.begin(),v.end());
        //serial << "max heap after push: " << v.front() << '\n';
        TRACE ("max heap after push: %d", v.front());

        std::sort_heap (v.begin(),v.end());

        //serial << "final sorted range :";
        for (unsigned i=0; i<v.size(); i++) {
            //serial << ' ' << v[i];
            TRACE ("%d", v[i]);
        }

        //serial << '\n';
        //return 0;
    }
};

void setup(void) {
    TRACE ("setup");
#if USE_DEBUG && defined(ARDUINO)
    Serial.begin(9600);
    // Wait for USB Serial.
    while (!Serial) {}

    // Read any input
    delay(200);
    while (Serial.read() >= 0) {}
#endif

    TestVector::RunTest();
    TestHeap::RunTest();
}

void loop(void) {
    TRACE ("loop");
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


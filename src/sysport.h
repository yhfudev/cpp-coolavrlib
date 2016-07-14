/**
 * @file    sysport.h
 * @brief   port the arduino programs to PC platform for debug
 * @author  Yunhui Fu (yhfudev@gmail.com)
 * @version 1.0
 * @date    2016-07-04
 * @copyright GPL
 */

#ifndef _SYS_PORT_H
#define _SYS_PORT_H 1

#ifndef DEBUG
#define DEBUG 0
#endif

// gcc 4.6.0
#if GCC_VERSION < 40600
#define nullptr NULL
//#define intptr_t int
#endif

#include <assert.h>

#if defined(ARDUINO)

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#include <Wire.h>

#if DEBUG
//#define TRACE(fmt, ...) {char buf[200]; snprintf (buf, sizeof(buf), "%d [%s()] " fmt " {ln:%d, fn:" __FILE__ "}\n", millis(), __func__, ##__VA_ARGS__, __LINE__); Serial.println(buf); }
//#define TRACE(fmt, ...) {static const PROGMEM char CONSTSTR[] = "%d %d " fmt " {ln:%d, fn:" __FILE__ "}\n"; char buf[200]; snprintf_P (buf, sizeof(buf), CONSTSTR, millis(), ##__VA_ARGS__, __LINE__); Serial.println(buf); }
#define TRACE(fmt, ...) {static const PROGMEM char CONSTSTR[] = "%d %d " fmt " {ln:%d}\n"; char buf[200]; snprintf_P (buf, sizeof(buf), CONSTSTR, millis(), ##__VA_ARGS__, __LINE__); Serial.println(buf); }
#else
#define TRACE(...)
#endif

#else

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#if DEBUG
#define TRACE(fmt, ...) fprintf (stdout, "%d [%s()] " fmt " {ln:%d, fn:" __FILE__ "}\n", millis(), __func__, ##__VA_ARGS__, __LINE__)
#else
#define TRACE(...)
#endif

#ifdef __WIN32__                // or whatever
#define PRIiSZ "ld"
#define PRIuSZ "Iu"
#else
#define PRIiSZ "zd"
#define PRIuSZ "zu"
#endif
#define PRIiOFF "lld"
#define PRIuOFF "llu"


#define PROGMEM

#define constrain(x,a,b) (((x)<(a))?(a):((x)>(b)?(b):(x)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define dtostrf(val, width, prec, buf) sprintf (buf, "%" # width "." # prec "f", (val))

extern unsigned long millis(void);

#define LOW  0
#define HIGH 1
#define pinMode(pin, mode)

#define digitalWrite(pin, val) TRACE("digitalWrite(" # pin  ", " # val ")")
#define analogRead(pin) (0)

extern int digitalRead(uint8_t);

//#define analogWrite(pin,val) TRACE("analogWrite(" # pin  ", " # val ")")
extern void analogWrite (uint8_t pin, int val);

class PcWire {
public:
    void begin() {}
    void begin(uint8_t I2C_SLAVE_ADDR) {}
    void onReceive( void (*)(int) ) {}
    void onRequest( void (*)(void) ) {}

    uint8_t read(void) {return 0;}
    void write(uint8_t *buf, size_t sz) { }
    void write(uint8_t val) { }
    void beginTransmission (uint8_t pin) {}
    int requestFrom(uint8_t addr, size_t sz) { return 0; }
    void endTransmission() {}
};
static PcWire Wire;

class PcSerial {
public:
    void begin(int baud) {}
    void println(int a) {}
    void println(float a) {}
};
static PcSerial Serial;

#if defined(U8G_RASPBERRY_PI)
#include <unistd.h>
#define delay(a) usleep((a) * 1000)
#else
#undef USE_SDL
#define USE_SDL 1
#define SDL_Delay(a) usleep(a)
#define delay(a) SDL_Delay((a)*1000)
#endif

#endif

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
//#define pgm_read_word_near(a) *((uint16_t *)(a))
#define pgm_read_word_near(a) (*(a))
#define pgm_read_byte_near(a) *((uint8_t *)(a))
#define memcpy_P memcpy
#define strcat_P strcat
#endif

#ifndef LED_BUILTIN
#ifdef __AVR_ATtiny85__
#define LED_BUILTIN 1
#endif
#else
#define LED_BUILTIN 13
#endif

#endif // _SYS_PORT_H


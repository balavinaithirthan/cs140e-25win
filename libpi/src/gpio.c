/*
 * Implement the following routines to set GPIO pins to input or
 * output, and to read (input) and write (output) them.
 *  - DO NOT USE loads and stores directly: only use GET32 and
 *    PUT32 to read and write memory.
 *  - DO USE the minimum number of such calls.
 * (Both of these matter for the next lab.)
 *
 * See rpi.h in this directory for the definitions.
 */
#include "gpio.h"
#include "rpi.h"
#include <stdio.h>

// see broadcomm documents for magic addresses.
//
// if you pass addresses as:
//  - pointers use put32/get32.
//  - integers: use PUT32/GET32.
//  semantics are the same.
enum {
  GPIO_BASE = 0x20200000,
  gpio_set0 = (GPIO_BASE + 0x1C),
  gpio_clr0 = (GPIO_BASE + 0x28),
  gpio_lev0 = (GPIO_BASE + 0x34)

  // <you may need other values.>
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!

void gpio_set_function(unsigned int pin, gpio_func_t function) {
  unsigned pinValue = 0;
  if (pin >= 32 && pin != 47) {
    return;
  }
  switch (function) {
  case GPIO_FUNC_INPUT:
    pinValue = GPIO_FUNC_INPUT;
    break;
  case GPIO_FUNC_OUTPUT:
    pinValue = GPIO_FUNC_OUTPUT;
    break;
  case GPIO_FUNC_ALT0:
    pinValue = GPIO_FUNC_ALT0;
    break;
  case GPIO_FUNC_ALT1:
    pinValue = GPIO_FUNC_ALT1;
    break;
  case GPIO_FUNC_ALT2:
    pinValue = GPIO_FUNC_ALT2;
    break;
  case GPIO_FUNC_ALT3:
    pinValue = GPIO_FUNC_ALT3;
    break;
  case GPIO_FUNC_ALT4:
    pinValue = GPIO_FUNC_ALT4;
    break;
  case GPIO_FUNC_ALT5:
    pinValue = GPIO_FUNC_ALT5;
    break;
  default:
    // error function value
    return;
  }
  int primaryPinNumber = pin / 10;
  int secondaryPinNumber = pin % 10;
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += primaryPinNumber;
  unsigned value = get32(pinAddr);
  unsigned mask = 0x00000007 << (secondaryPinNumber * 3); // 111
  value &= ~mask;
  value |= pinValue << (secondaryPinNumber * 3);
  put32(pinAddr, value);
  return;
}

void gpio_set_output(unsigned pin) { gpio_set_function(pin, GPIO_FUNC_OUTPUT); }
// set <pin> to input.
void gpio_set_input(unsigned pin) { gpio_set_function(pin, GPIO_FUNC_INPUT); }

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
  if (pin >= 32 && pin != 47) // Check if pin is valid
    return;
  unsigned pinOffset = pin; // 0-31 is just pin
  unsigned gpioOffset = 7;  // 0-31
  if (pin >= 32) {
    gpioOffset = 8;
    pinOffset = pin - 32; // pin 32 is pin 0 for example
  }
  // unsigned *pinAddr = (unsigned *)gpio_set0;
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += gpioOffset;
  put32(pinAddr, 1 << pinOffset);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
  if (pin >= 32 && pin != 47) // Check if pin is valid
    return;
  unsigned pinOffset = pin; // 0-31 is just pin
  unsigned gpioOffset = 10; // 0-31
  if (pin >= 32) {
    gpioOffset = 11;
    pinOffset = pin - 32; // pin 32 is pin 0 for example
  }
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += gpioOffset;
  put32(pinAddr, 1 << pinOffset);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
  if (v)
    gpio_set_on(pin);
  else
    gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// return the value of <pin>
int gpio_read(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return -1;
  unsigned pinOffset = pin; // 0-31 is just pin
  unsigned gpioOffset = 13; // 0-31
  if (pin > 31) {
    gpioOffset = 14;
    pinOffset = pin - 32; // pin 32 is pin 0 for example
  }
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += gpioOffset;
  unsigned value = get32(pinAddr);
  unsigned mask = 1 << pinOffset;
  value &= mask;
  if (value > 0) {
    return DEV_VAL32(1);
  }
  return DEV_VAL32(0);
}

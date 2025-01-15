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
#include "rpi.h"

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

// works for 0-50
void gpio_set_output(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return;
  int primaryPinNumber = pin / 10;
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += primaryPinNumber;
  unsigned value = *pinAddr;
  unsigned mask = 0x00000007;
  value &= ~mask;
  value |= 0x00000001;
  put32(pinAddr, value);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return;
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  unsigned value = *pinAddr;
  unsigned bits = 0x00000001 << pin;
  value &= ~0x00100000;
  value |= 0x00100000;
  put32(pinAddr, value);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
  if (pin >= 32 && pin != 47)
    return;
  unsigned *pinAddr = (unsigned *)GPIO_BASE;
  pinAddr += 10;
  unsigned value = *pinAddr;
  value &= ~0x00100000;
  value |= 0x00100000;
  put32(pinAddr, value);
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

// set <pin> to input.
void gpio_set_input(unsigned pin) {
  // implement.
}

// return the value of <pin>
int gpio_read(unsigned pin) {
  unsigned v = 0;

  // implement!

  return v;
}

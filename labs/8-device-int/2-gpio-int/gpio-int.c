// engler, cs140 put your gpio-int implementations in here.
#include "gpio.h"
#include "rpi.h"
// in libpi/include: has useful enums.
#include "rpi-interrupts.h"

#include <stdint.h>
#include <stdio.h>

#define GPIO_BASE 0x20200000

enum {
    GPRENn_RISING = 0x2020004C,
    GPRENn_FALLING = 0x20200058,
    GPEDSn = 0x20200040,
    IRQ_PENDING = 0x2000b208,
    ENABLE_IRQ = 0x2000b214,
    //GPLEV0 = 0x20200034
};

/*
How interrupts work?
The entire system only gets one interrupt (on or off) and correspondingly
jumps to the causing vector table.  
We need to mutliplex that interrupt
How??
First, we the gpio handler has interrupt information that is sandboxed to itself
- it uses falling edge/rising edge detect and will set an event accordingly on GSEPn register

This event register corresponds to one bit on the IRQ Pending table.
From the pending table, we only listen to it if that same bit is available on the enable table!

*/



uint32_t generate_mask_new(uint32_t start, uint32_t end) {
    return ((1U << (end - start)) - 1) << start;
}

void read_modify_write_new(uint32_t addr, uint32_t value, uint32_t bitNumberStart, uint32_t bitNumberEnd) {
    uint32_t originalValue = GET32(addr);
    uint32_t bitMask = generate_mask_new(bitNumberStart, bitNumberEnd);
    originalValue &= ~bitMask;
    originalValue |= value << bitNumberStart;
    PUT32(addr, originalValue);
}

// set GPIO <pin> on.
void gpio_set_on_new(unsigned pin, uintptr_t base) {
  if (pin >= 32) // Check if pin is valid
    return;
  unsigned pinOffset = pin; // 0-31 is just pin
  unsigned *pinAddr = (unsigned *)base;
  put32(pinAddr, 1 << pinOffset);
}

// set GPIO <pin> off
void gpio_set_off_new(unsigned pin, uintptr_t base) {
  if (pin >= 32) // Check if pin is valid
    return;
  unsigned pinOffset = pin; // 0-31 is just pin  }
  unsigned *pinAddr = (unsigned *)base;
  put32(pinAddr, 1 << pinOffset);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write_new(unsigned pin, unsigned v, uintptr_t base) {
  if (v)
    gpio_set_on_new(pin, base);
  else
    gpio_set_off_new(pin, base);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// return the value of <pin>
int gpio_read_new(unsigned pin, uintptr_t base) {
  if (pin >= 32)
    return -1;
  unsigned pinOffset = pin; // 0-31 is just pin
  unsigned *pinAddr = (unsigned *)base;
  unsigned value = get32(pinAddr);
  unsigned mask = 1 << pinOffset;
  value &= mask;
  if (value > 0) {
    return 1;
  }
  return 0;
}



// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    dev_barrier();
    uint32_t val = get32((uint32_t*)IRQ_PENDING) & (1 << (49 - 32)); // TOOD: where this be actually?
    dev_barrier();
    return val;
}
// TODO: don't understand this
// we do get32() instead of raw dereference because volatile prevents movement of instructions and optimizations

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)

void gpio_int_rising_edge(unsigned pin) {
    if(pin>=32)
        return;
    dev_barrier();
    read_modify_write_new(GPRENn_RISING, 1, pin, pin+1);
    dev_barrier();
    gpio_write_new(49-32, 1, ENABLE_IRQ);
    dev_barrier();

}



// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    if(pin>=32)
        return;
    dev_barrier();
    read_modify_write_new(GPRENn_FALLING, 1, pin, pin + 1);
    dev_barrier();
    gpio_write_new(49-32, 1, ENABLE_IRQ);
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    if(pin>=32)
        return 0;
    dev_barrier();
    uint32_t val = gpio_read_new(pin, GPEDSn);
    dev_barrier();
    return val;
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    if(pin>=32)
        return;
    dev_barrier();
    gpio_write_new(pin, 1, GPEDSn);
    dev_barrier();
}
// bootloader helpers and interface.  The file that includes this
// must provide three routines:
//   uint8_t boot_get8(void): write 8 bits on network.
//   void boot_put8(uint8_t b): read 8 bits on network.
//   boot_has_data(): returns 1 if there is data on the network.
//
// we could provide these as routines in a structure (poor-man's
// OO) but we want the lowest overhead possible given that we want
// to be able to run without interrupts and finite sized input buffers.
//
// The first half of the file you don't have to modify (but can!).
// The second is your bootloader.
//
// much more robust than xmodem, which seems to have bugs in terms of
// recovery with inopportune timeouts.

#ifndef __GETCODE_H__
#define __GETCODE_H__
#include "boot-crc32.h" // has the crc32 implementation.
#include "boot-defs.h"  // protocol opcode values.
#include <stdint.h>

/***************************************************************
 * Helper routines.  You shouldn't need to modify these for
 * the initial version.
 */

// return a 32-bit: little endien order.
//
// NOTE: as-written, the code will loop forever if data does
// not show up or the hardware dropped bytes (e.g. b/c
// you didn't read soon enough)
//
// After you get the simple version working, you should fix
// it by making a timeout version.
static inline uint32_t boot_get32(void) {
  uint32_t u = boot_get8();
  u |= boot_get8() << 8;
  u |= boot_get8() << 16;
  u |= boot_get8() << 24;
  return u;
}

// send 32-bits on network connection.  (possibly let the
// network implementation override for efficiency)
static inline void boot_put32(uint32_t u) {
  boot_put8((u >> 0) & 0xff);
  boot_put8((u >> 8) & 0xff);
  boot_put8((u >> 16) & 0xff);
  boot_put8((u >> 24) & 0xff);
}

// send a string <msg> to the unix side to print.
// message format:
//  [PRINT_STRING, strlen(msg), <msg>]
//
// DANGER DANGER DANGER!!!  Be careful WHEN you call this!
// DANGER DANGER DANGER!!!  Be careful WHEN you call this!
// DANGER DANGER DANGER!!!  Be careful WHEN you call this!
// Why:
//   1. We do not have interrupts.
//   2. The UART RX FIFO can only hold 8 bytes before it starts
//      dropping them.
//   3. So if you print at the same time the laptop sends data,
//      you will likely lose some, leading to weird bugs.  (multiple
//      people always do this and waste a long time)
//
// So: only call boot_putk right after you have completely
// received a message and the laptop side is quietly waiting
// for a response.
static inline void boot_putk(const char *msg) {
  // send the <PRINT_STRING> opcode
  boot_put32(PRINT_STRING);

  unsigned n = strlen(msg);
  // send length.
  boot_put32(strlen(msg));

  // send the bytes in the string [we don't include 0]
  for (n = 0; msg[n]; n++)
    boot_put8(msg[n]);
}

// example of how to use macros to get file and lineno info
// if we don't do the LINE_STR() hack (see assert.h),
// what happens?
#define boot_todo(msg)                                                         \
  boot_err(BOOT_ERROR, __FILE__ ":" LINE_STR() ":TODO:" msg "\n")

// send back an error and die.   note: we have to flush the output
// otherwise rebooting could trash the TX queue (example of a hardware
// race condition since both reboot and TX share hardware state)
static inline void boot_err(uint32_t error_opcode, const char *msg) {
  boot_putk(msg);
  boot_put32(error_opcode);
  uart_flush_tx();
  rpi_reboot();
}

/*****************************************************************
 * Your bootloader implementation goes below.
 *
 * NOTE: f you need to print: use boot_putk.  only do this
 * when you are *not* waiting for data or you'll lose it.
 */

// wait until:
//   (1) there is data (uart_has_data() == 1): return 1.
//   (2) <timeout> usec expires, return 0.
//
// look at libpi/staff-src/timer.c
//   - call <timer_get_usec()> to get usec
//   - look at <delay_us()> : for how to correctly
//     wait for <n> microseconds given that the hardware
//     counter can overflow.
static unsigned has_data_timeout(unsigned timeout) {
  uint32_t s = timer_get_usec();
  while (1) {
    if (uart_has_data()) {
      return 1;
    }
    uint32_t e = timer_get_usec();
    if ((e - s) >= timeout)
      return 0;
  }
}

// iterate:
//   - send GET_PROG_INFO to server
//   - call <has_data_timeout(<usec_timeout>)>
//       - if =1 then return.
//       - if =0 then repeat.
//
// NOTE:
//   1. make sure you do the delay right: otherwise you'll
//      keep blasting <GET_PROG_INFO> messages to your laptop
//      which can end in tears.
//   2. rember the green light that blinks on your ttyl device?
//      Its from this loop (since the LED goes on for each
//      received packet) TODO:
static void wait_for_data(unsigned usec_timeout) {
  //! has_data_timeout(usec_timeout) .  has_data_timeout(usec_timeout)
  //!
  boot_put32(GET_PROG_INFO);
  while (!has_data_timeout(usec_timeout)) {
    boot_put32(GET_PROG_INFO);
  }
}

// TODO: What if put bytes remaining inside buffer?

// IMPLEMENT this routine.
//
// Simple bootloader: put all of your code here.
uint32_t get_code(void) {
  // 0. keep sending GET_PROG_INFO every 300ms until
  // there is data: implement this.
  wait_for_data(300 * 1000);
  uint32_t progInfo = boot_get32();
  // boot_putk("got put prog info");
  /****************************************************************
   * Add your code below: 2,3,4,5,6
   */

  // 2. expect: [PUT_PROG_INFO, addr, nbytes, cksum]
  //    we echo cksum back in step 4 to help debugging.
  uint32_t address = boot_get32();
  if (address != ARMBASE) {
    boot_err(2, "The value is not ARMBASE");
    rpi_reboot();
  }
  uint32_t nbytes = boot_get32();
  uint32_t cksum = boot_get32();
  // boot_putk("got both");
  // 3. If the binary will collide with us, abort with a BOOT_ERROR.
  //
  //    check that the sent code (<base_addr> through
  //    <base_addr>+<nbytes>) doesn't collide with
  //    the bootloader code using the address of <PUT32>
  //    (the first code address we need) to __prog_end__
  //    (the last).
  //
  //    refer back to:
  //       - your gprof lab code
  //       - libpi/include/memmap.h
  //       - libpi/memmap
  //    for definitions.

  uint32_t start = address;
  uint32_t end = address + nbytes;
  unsigned int put32_addr = (unsigned int)&PUT32;
  if (start > put32_addr || end > put32_addr) { // should be more robust
    boot_err(1, "Address issue");
  }

  // 4. send [GET_CODE, cksum] back.
  // boot_todo("send [GET_CODE, cksum] back\n");
  boot_put32(GET_CODE);
  boot_put32(cksum);

  // 5. we expect: [PUT_CODE, <code>]
  //  read each sent byte and write it starting at
  //  <addr> using PUT8
  //
  // common mistake: computing the offset incorrectly.
  uint32_t putCode = boot_get32();
  if (putCode != PUT_CODE) {
    boot_err(1, "Did not receive a PUT CODE");
  }
  // uint8_t buf[nbytes];
  // boot_putk("putting code");

  for (int i = 0; i < nbytes; i++) {
    uint8_t byte = boot_get8();
    uintptr_t pointer = (uintptr_t)(start + i);
    PUT8((uint32_t)(pointer), byte);
  }
  // 6. verify the cksum of the copied code using:
  //         boot-crc32.h:crc32.
  //    if fails, abort with a BOOT_ERROR.
  if (crc32((void *)start, nbytes) != cksum) {
    boot_err(1, "The checksum is incorrect");
    rpi_reboot();
  }
  // boot_putk("checksum correct");

  // 7. send back a BOOT_SUCCESS!
  boot_putk("<Bala>: success: Received the program!");
  // boot_todo("fill in your name above");

  // woo!
  boot_put32(BOOT_SUCCESS);

  // We used to have these delays to stop the unix side from getting
  // confused.  I believe it's b/c the code we call re-initializes the
  // uart.  Instead we now flush the hardware tx buffer.   If this
  // isn't working, put the delay back.  However, it makes it much faster
  // to test multiple programs without it.
  delay_ms(500);
  uart_flush_tx();
  return address;
}
#endif

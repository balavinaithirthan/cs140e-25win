
// simple mini-uart driver: implement every routine 
// with a <todo>.
//
// NOTE: 
//  - from broadcom: if you are writing to different 
//    devices you MUST use a dev_barrier().   
//  - its not always clear when X and Y are different
//    devices.
//  - pay attenton for errata!   there are some serious
//    ones here.  if you have a week free you'd learn 
//    alot figuring out what these are (esp hard given
//    the lack of printing) but you'd learn alot, and
//    definitely have new-found respect to the pioneers
//    that worked out the bcm eratta.
//
// historically a problem with writing UART code for
// this class (and for human history) is that when 
// things go wrong you can't print since doing so uses
// uart.  thus, debugging is very old school circa
// 1950s, which modern brains arne't built for out of
// the box.   you have two options:
//  1. think hard.  we recommend this.
//  2. use the included bit-banging sw uart routine
//     to print.   this makes things much easier.
//     but if you do make sure you delete it at the 
//     end, otherwise your GPIO will be in a bad state.
//
// in either case, in the next part of the lab you'll
// implement bit-banged UART yourself.
#include "rpi.h"

// change "1" to "0" if you want to comment out
// the entire block.
#if 1 
//*****************************************************
// We provide a bit-banged version of UART for debugging
// your UART code.  delete when done!
//
// NOTE: if you call <emergency_printk>, it takes 
// over the UART GPIO pins (14,15). Thus, your UART 
// GPIO initialization will get destroyed.  Do not 
// forget!   

// header in <libpi/include/sw-uart.h>
#include "sw-uart.h"
static sw_uart_t sw_uart;

// if we've ever called emergency_printk better
// die before returning.
static int called_sw_uart_p = 0;

// a sw-uart putc implementation.
static int sw_uart_putc(int chr) {
    sw_uart_put8(&sw_uart,chr);
    return chr;
}

// call this routine to print stuff. 
//
// note the function pointer hack: after you call it 
// once can call the regular printk etc.
static void emergency_printk(const char *fmt, ...) {
    // track if we ever called it.
    called_sw_uart_p = 1;


    // we forcibly initialize each time it got called
    // in case the GPIO got reset.
    // setup gpio 14,15 for sw-uart.
    sw_uart = sw_uart_default();

    // all libpi output is via a <putc>
    // function pointer: this installs ours
    // instead of the default
    rpi_putchar_set(sw_uart_putc);

    printk("NOTE: HW UART GPIO is in a bad state now\n");

    // do print
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

}

#undef todo
#define todo(msg) do {                      \
    emergency_printk("%s:%d:%s\nDONE!!!\n",      \
            __FUNCTION__,__LINE__,msg);   \
    rpi_reboot();                           \
} while(0)

// END of the bit bang code.
#endif

enum {
    AUXIRQ = 0x20215000,
    AUXENB = 0x20215004,
    AUX_MU_IO_REG = 0x20215040,
    AUX_MU_IER_REG = 0x20215044,
    AUX_MU_IIR_REG = 0x20215048,
    AUX_MU_LCR_REG = 0x2021504C,
    AUX_MU_LSR_REG = 0x20215054,
    AUX_MU_CNTL_REG = 0x20215060,
    AUX_MU_STAT_REG = 0x20215064,
    AUX_MU_BAUD_REG = 0x20215068,
};


uint32_t generate_mask(uint32_t start, uint32_t end) {
    return ((1U << (end - start)) - 1) << start;
}

uint32_t read_modify_write(uint32_t addr, uint32_t value, uint32_t bitNumberStart, uint32_t bitNumberEnd) {
    uint32_t originalValue = GET32(addr);
    uint32_t bitMask = generate_mask(bitNumberStart, bitNumberEnd);
    originalValue &= ~bitMask;
    originalValue |= value << bitNumberStart;
    return originalValue;
}


// void debugger() {
//     hw_uart_disable();
//     // use pin 14 for tx, 15 for rx
//     sw_uart_t u = sw_uart_init(14, 15, 115200);
//     // print in the most basic way.
//     sw_uart_put8(&u, 'h');
//     sw_uart_put8(&u, 'e');
//     sw_uart_put8(&u, 'l');
//     sw_uart_put8(&u, 'l');
//     sw_uart_put8(&u, 'o');
//     sw_uart_put8(&u, '\n');
//     sw_uart_put8(&u, '\n');
//     sw_uart_put8(&u, '\n');
// }
//*****************************************************
// the rest you should implement.

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // NOTE: make sure you delete all print calls when
    // done!
    // emergency_printk("start here\n"); // TODO: why does this trash hardware uart??

    // perhaps confusingly: at this point normal printk works
    // since we overrode the system putc routine.
    // printk("write UART addresses in order\n");
    dev_barrier();

    // gpio_set_output(14);
    // gpio_set_input(15); // EXplain function
    gpio_set_function(14, GPIO_FUNC_ALT5);
    gpio_set_function(15, GPIO_FUNC_ALT5);

    dev_barrier();

    dev_barrier();
    // while ((GET32(AUXIRQ) & 1) == 1); // miniUART interrupt pending
    read_modify_write(AUXENB, 1, 0, 1); //set miniUART on
    // read_modify_write(AUX_MU_CNTL_REG, 0, 0, 2); // disable transmit and recv
    PUT32(AUX_MU_CNTL_REG, 0); // disable transmit and recv

    // read_modify_write(AUX_MU_IER_REG, 0, 1, 2); // Disable receive interrupt 
    // read_modify_write(AUX_MU_IER_REG, 0, 0, 1); // Disable transmit interrupt
    PUT32(AUX_MU_IER_REG, 0); // Disable receive and transmit interrupt

    // read_modify_write(AUX_MU_IIR_REG, 1, 2, 3); // clear transmit FIFO
    // read_modify_write(AUX_MU_IIR_REG, 1, 1, 2); // clear receive FIFO
    PUT32(AUX_MU_IIR_REG, 6); // clear transmit and receive FIFO

    //read_modify_write(AUX_MU_LCR_REG, 3, 1, 3); // put in 8 bit mode
    PUT32(AUX_MU_LCR_REG, 3); // put in 8 bit mode

    uint32_t baudrate = 270;
    //read_modify_write(AUX_MU_BAUD_REG, baudrate, 0, 16); // set baudrate 
    PUT32(AUX_MU_BAUD_REG, baudrate); // set baudrate

    //read_modify_write(AUX_MU_CNTL_REG, 3, 0, 2); // enable transmit and recv
    PUT32(AUX_MU_CNTL_REG, 3); // enable transmit and recv

    // read_modify_write(AUXENB, 1, 0, 1); //set miniUART on

    // disable transmit interrupts
    // 

    dev_barrier();

    // delete everything to do w/ sw-uart when done since
    // it trashes your hardware state and the system
    // <putc>.
    demand(!called_sw_uart_p, 
        delete all sw-uart uses or hw UART in bad state);
}

// disable the uart: make sure all bytes have been proc
// 
void uart_disable(void) {
    read_modify_write(AUX_MU_CNTL_REG, 0, 0, 2); // disable transmit and recv
    read_modify_write(AUXENB, 0, 0, 1); //set miniUART off
}

// returns one byte from the RX (input) hardware
// FIFO.  if FIFO is empty, blocks until there is 
// at least one byte.
int uart_get8(void) { 
    while ((GET32(AUX_MU_LSR_REG) & 1) == 0); // rx data not ready
    uint8_t ch;
    return (GET32(AUX_MU_IO_REG) & 0xFF);
//    todo("must implement\n"); 
}

// returns 1 if the hardware TX (output) FIFO has room
// for at least one byte.  returns 0 otherwise.
int uart_can_put8(void) {
    return (GET32(AUX_MU_STAT_REG) & 2) >> 1;
}

// put one byte on the TX FIFO, if necessary, waits
// until the FIFO has space.
int uart_put8(uint8_t c) {
    dev_barrier(); // TODO: Why is their a dev_barrier
    // while ((GET32(AUX_MU_LSR_REG) & (1 << 5)) == 0) ; // tx fifo full
    while (!uart_can_put8()) {
        rpi_wait();
    };
    PUT32(AUX_MU_IO_REG, c); // why is this also 7:0 TODO
    dev_barrier(); // TODO 
    return 1;
}

// returns:
//  - 1 if at least one byte on the hardware RX FIFO.
//  - 0 otherwise
int uart_has_data(void) {
    return (GET32(AUX_MU_STAT_REG) & 1);
}

// returns:
//  -1 if no data on the RX FIFO.
//  otherwise reads a byte and returns it.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// returns:
//  - 1 if TX FIFO empty AND idle.
//  - 0 if not empty.
int uart_tx_is_empty(void) {
    todo("must implement\n");
}

// return only when the TX FIFO is empty AND the
// TX transmitter is idle.  
//
// used when rebooting or turning off the UART to
// make sure that any output has been completely 
// transmitted.  otherwise can get truncated 
// if reboot happens before all bytes have been
// received.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        rpi_wait();
}
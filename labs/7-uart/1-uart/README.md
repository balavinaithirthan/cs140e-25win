### UART

Implement `uart.c` using the Broadcom document.

  - Before you start: run `make check` --- it should pass.  
    Then change the `Makefile` from:

        # COMMON_SRC += uart.c

    to
        COMMON_SRC += uart.c

    so that it works with the local uart.

    You probably want to start with the first test, second, etc.


  - you should be able to copy `uart.c` into `libpi/src`, and modify
    `libpi/Makefile` to use that instead of our staff copy.


  - recompile your bootloader and reinstall.  your checkoff from the last
    lab should still pass.

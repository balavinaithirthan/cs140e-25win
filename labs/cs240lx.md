## CS240lx

If you liked 140e, consider taking cs240lx next quarter. 
  - it's fun.
  - it's Joe's last quarter before graduation.
  - we currently have a record number enrolled

Same rough format: 
  - still 2 days a week.
  - still hardware, low level code.
  - still pizza.

Differences:
  - usually stop around 1030 or so, vs 1230.
  - is more conceptual based.
      - 140e must cover execution, vm, fs, some network.
      - 240lx assumes you know that stuff, and use them to do
        stuff.
  - substitution for cs240 (paper reading).
  - usually its the people who find 140e fun (+/-).

Topics:
  - cool tricks i picked up over 3+ decades. or seem not well covered.
  - depends alot on what people are interested in.  can be
    OS heavy, or more device heavy, or more project heavy
    depending.  let us know what kind of stuff you are into.
  - Probably at least one new board (pico, different riscv).

Always generate executable code at runtime
  - even more low level than inline assembly.
  - self-modifying code is the ultimate sleazy hack.
  - used to make fast(er) interpreted languages (JIT for
    javascript, etc), fast dynamic intstrumentation (valgrind),
    and old school virtual machines (vmware).
  - can do all sorts of speed hacks by specializing using runtime
    information.  compiling data structures to code.  etc.

Also ways do a custom pcb:
  - parthiv historically comes in to do a week of custom pcb labs so
    can make your own cool boards.


Always build a bunch of tools using memory faults and single-stepping.
  - Eraser race detector 
  - Purify memory checker 
  - Advanced interleave checker.  
  - instruction level profiler measuring cycles, cache misses,
    etc.

If we have more kernel hackers some subset of:
  - network bootloader
  - make an actual OS that ties all the stuff together.
  - small distributed system
  - distributed file system
  - actual clean fat32 r/w so can do distributed file system,
    firmware updates.
  - processes that migrate from one pi to another.
  - FUSE file system interface.

Speed:
  - make low level operations (exceptions, fork/wait, pipe
    ping pong) 10-100x faster than laptop.  i think this is
    feasible but haven't done, so am interested.
  - overclocking the pi and seeing how fast can push it.
  - push the NRF using interrupts and concurrent tx, rx to 
    see how close can get to the hw limit.

  - make a digital analyzer (printk for electricity) where
    you get the error rate down to a small number of nanoseconds

different communication protocols: 
  - how fast can send/recv data over gpio pins b/n two pis?
  - over IR
  - over speaker/mic.
  - over light and camera (?)
  - lora?

Fun device labs:
  - this is the fun stuff, so we do it.
  - accelerometer, gyro
  - lidar
  - addressable light array
  - analog to digital converter.
  - lora so can send bytes 1km+

combining devices into standalone tools
  - acoustically reactive light display using mic, adc, addressable light
    array.  extend to multiple systems.
  - little osscilliscope using oled display, mic, adc

Lots of other stuf.
  - e.g., maybe some other languages (rust?  zig?)
  - static bug finder.

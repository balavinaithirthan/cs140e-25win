### What to do

This is the biggest step.  From the lab README:
  1. Copy your `step2/trojan-compiler.c` to here.
  2. Modify it to have a self-reproducing attack.  

     NOTE: the attack should *not* read in any attack file when it runs
     --- thus, you should not be calling open(), fopen() or getchar()
     on anything besides the input to the compiler.

  3. The Makefile shows a possible workflow but you can do your own.
     To use it you'll have to have `trojan-compiler2.c` and your attack
     in a file called `attack-seed.c`.

     But you don't have to do things that way: you can write your own.

  4. If your run `check.sh` it should pass.

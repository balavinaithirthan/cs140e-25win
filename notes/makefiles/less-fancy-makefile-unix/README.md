### Automatic robust makefile.

***NOTE: `prebuilt-obj.o` was built on linux so will fail if you run on
MacOS.  You can just run `gcc -c b.c -o prebuilt-b.o` if this happens
to generate it.***

This makefile is my current view of the "safest" approach to compiling
a big multi-language (C, asm) project that spans directories and putting
the results in a `BUILD_DIR` without the use of `eval`.  It has numerous
error-checking tricks.

Limits:
  1. Does not handle relative paths that go outside the
     makefile directory.

     See: `fancy-makefile-unix` for how to handle this problem.
     Likely a better approach overall.

  2. Long: but this is mostly b/c of comments and b/c it
     shows other tricks (that you can simply delete).

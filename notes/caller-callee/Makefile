NAME = caller-callee.c

include ../../libpi/defs.mk

CFLAGS :=  -O2 -march=armv6

all: $(NAME:.c=.o)

%.o: %.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@
	$(OD) -d   $*.o > $*.list
	cat $*.list

FORCE:

clean:
	rm -rf *~ tags *.o *.list Makefile.bak  *.d 

.PHONY: clean 

# stop make from deleting these suffix'd files.
# .PRECIOUS: %.list %.elf %.o

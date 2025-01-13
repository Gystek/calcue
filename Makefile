CC := gcc
LD := $(CC)

CFLAGS := -ansi -Wall -Wextra -Wwrite-strings -Wno-variadic-macros -O1 -Iinclude/
LDFLAGS :=

CSRC := src/main.c src/lexer.c src/dynarray.c
COBJ := $(CSRC:.c=.o)

all: calc
calc: $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f calc

clear: clean
	rm -f $(COBJ)

.PHONY: all clean clear

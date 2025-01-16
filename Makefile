CC := gcc
LD := $(CC)

CFLAGS := -std=c99 -Wall -Wextra -Wwrite-strings\
          -Wno-variadic-macros -O0 -Iinclude/\
          #-D_DUMP_VM -D_DISASSEMBLE\
          #-D_DISPLAY_LEXING #-D_DISPLAY_PARSING -D_DUMP_VM_EVERY_CYCLE
LDFLAGS :=

CSRC := src/main.c src/lexer.c src/dynarray.c src/parser.c src/resolver.c\
        src/compiler.c src/vm.c src/arith.c
COBJ := $(CSRC:.c=.o)

all: calc
calc: $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f calc

clear: clean
	rm -f $(COBJ)

.PHONY: all clean clear

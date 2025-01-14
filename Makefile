CC := gcc
LD := $(CC)

CFLAGS := -ansi -Wall -Wextra -Wwrite-strings -Wno-variadic-macros\
          -O1 -Iinclude/ -D_DISPLAY_LEXING -D_DISPLAY_PARSING
LDFLAGS :=

CSRC := src/main.c src/lexer.c src/dynarray.c src/parser.c src/resolver.c
COBJ := $(CSRC:.c=.o)

all: calc
calc: $(COBJ)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f calc

clear: clean
	rm -f $(COBJ)

.PHONY: all clean clear

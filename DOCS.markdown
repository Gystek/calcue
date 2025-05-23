Documentation
=============

This documentation file goes through most of `calcue`'s feature
(and also delves a bit into its design).

Installing
----------

`calcue` is installed using GNU Make.  You can customise `calcue`'s
behaviour by setting macros (using the `-D<macro>` flag).  The
following macros alter the calculator's behaviour when set:

- `_DISPLAY_LEXING`: prints the array of tokens after lexing.
- `_DISPLAY_PARSING`: prints the program's abstract syntax tree
after parsing.
- `_DISASSEMBLE`: prints the generated bytecode after compilation.
- `_DUMP_VM_EVERY_CYCLE`: makes the virtual machine display its
state after every opcode.
- `_DUMP_VM`: print the virtual machine state after execution.

Language concepts
-----------------

In the following section, code blocks indicate the code written by
the user, and the calculator's output is prefixed by `| `.

### Basic arithmetic

Basic arithmetical operations are unsurprising:

```
5 + 5
| => 10
```

```
7 - 3
| => 4
```

```
3 + 6 * 9
| => 57
```

```
3 ^ 2
| => 9
```

```
5 / 2
| => 2
```

As one can see, the division of one integer by another yields
a third integer. However:

```
5 / 2.0
| => 2.5
```

The same goes for `+`, `-`, `*` and `^`.

```
5 % 3
| => 2
```

The modulo operation is undefined on floating point numbers.  `x % y`
returns `x` if either one of `x` and `y` is not an integer.

The opposite of a number is obtained by applying the unary `-`
operator.

```
-5
| => -5
```

```
--5
| => 5
```

### Advanced arithmetic

Three "advanced" arithmetical functions are available at the time:
`log`, `exp` and `sqrt`.

```
log 10
| => 2.302585
```

```
exp 10
|=> 21950.378849
```

```
sqrt 81
|=> 9.0
```

### Variables

Variables are declared using the following syntax:

```
x := 5
```

Variables can be shadowed:

```
x := 5
x := 6
print x
| 6
```

Variables can be used where any expression would be expected:

```
x := 5
y := 6 + x
```

### Interacting with the user

Information can be given to the user using two different functions:

- `print` displays a value to the standard output:

```
print (5 + 6)
| 11
```

- `dump` displays all variables and their value to the standard
output:

```
x := 5
y := 6
dump
| x = 5 ; y = 6 ;
```

A value can be read from the standard input using the `read`
function:

```
x := read
print x
| (input) 5
| 5
```

### Boolean operations

There is no difference between boolean values and numerical ones.
The rule is: `0` and `0.0` mean `false` and any other value means
`true`.

The following operations yield a pseudo-boolean value: `=`, `<>`
(checking for unequality), `<`, `>`, `<=`, `>=`.

The unary operator `~` performs a boolean "not":

```
x := 5
print ~x
print ~~x
| 0
| 1
```

The operators `|` and `&` respectively perform boolean "or" and
"and":

```
print (0 | 1)
print (0 & 1)
| 1
| 0
```

### Control flow

There are two main control flow structures: `if` and `while`.

`if` takes a condition, a sequence of statements to execute
if the condition is verified and (optionally) a sequence of
statements to execute if the condition is *not* verified:

```
x := 5
if x = 5 then
  print 1
end
| 1
```

```
x := 5
if x = 6 then
  print 1
else
  print 0
end
| 0
```

`while` takes a condition and a sequence of statements and executes
the latter until the condition is no longer met:

```
i := 0
while i < 5 do
  i := i + 1
  print i
end
| 1
| 2
| 3
| 4
| 5
```

Appendices
----------

### A. Language grammar

Below is the grammar for the calculator language expressed in
EBNF. An unlimited amount of space characters (` `, `\t`, `\r`)
may separate two components of a rule (where a comma is written):

```ebnf
program = [ statement, { separator, { separator }, statement } ] ;
statement = binding | if-then | while-do | expr ;
binding = identifier, ":=", expr ;
if-then = "if", expr, "then", { separator }, program,
          [ "else",  { separator }, program ], "end" ;
while-do = "while", expr, "do", program, "end" ;
expr = logop, { "&" | "|", logop } ;
logop = eqneq, { "=" | "<>", eqneq } ;
eqneq = cmp, { ">" | "<" | "<=" | ">=", cmp } ;
cmp = sum, { "+" | "-", sum } ;
sum = mul, { "*" | "/" | "%", mul } ;
mul = pow, { "^", pow } ;
pow = ( ? primitive function name ?, { expr } ) | unary ;
unary = [ "-" | "~" ], literal ;
literal = integer | float | identifier | ( "(", expr, ")" ) ;
integer = ? what is accepted by `strtol` as an integer ?
float = ? what is accepted by `strtod` as a floating-point number ?
identifier = ? a..z A..Z ? | "_", { ? a..z A..Z 0..9 ? | "_" } ;
separator = "\n" | ";" | ? EOF ? ;
```

### B. Compiler and bytecode

This section describes the functionning of the compiler and the
structure of the bytecode.

Numbers (addresses, integers, floating-point numbers) are
decomposed in little-endian order.

The maximum bytecode size was fixed at 0xFFFF opcodes. However,
addresses are stored on four bytes.

The compiler transforms variable names into indices. During
this process, it also checks for use of undeclared variables (in
which case it raises an error).

The opcodes are as follows:

| opcode | code | bytes | description                                                                                                                                                                |
|--------|------|-------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `HLT`  | 0x00 | 0     | halts the execution, prints the stack top value (if applicable) and clears the stack                                                                                       |
| `INT`  | 0x01 | 4     | pushes the following integer onto the stack                                                                                                                                |
| `FLT`  | 0x02 | 8     | pushes the following floating-point number onto the stack                                                                                                                  |
| `STR`  | 0x03 | 4     | stores the stack top value into the following variable                                                                                                                     |
| `LOD`  | 0x04 | 4     | loads the following variable onto the stack                                                                                                                                |
| `PRM`  | 0x05 | 4     | calls the following primitive with the values on the stack                                                                                                                 |
| `JMP`  | 0x06 | 4     | jumps to the following address                                                                                                                                             |
| `JPI`  | 0x07 | 4     | jumps to the following address if the stack top value equals `true` when interpreted as a boolean value                                                                    |
| `AND`  | 0x08 | 0     | performs a logical "and" on the stack top values                                                                                                                           |
| `OOR`  | 0x09 | 0     | performs a logical "or" on the stack top values                                                                                                                            |
| `NOT`  | 0x0A | 0     | performs a logical "not" on the stack top value                                                                                                                            |
| `CEQ`  | 0x0B | 0     | checks for equality of the stack top values - pushes a true value if they are equal, 0 else                                                                                |
| `ORD`  | 0x0C | 0     | checks the order of the stack top values - pushes -1 if the top value is lesser than the next one, 0 if they are equal and 1 if the top value is greater than the next one |
| `ADD`  | 0x0D | 0     | self-explanatory                                                                                                                                                           |
| `SUB`  | 0x0E | 0     | idem                                                                                                                                                                       |
| `NEG`  | 0x0F | 0     | pushes the opposite of the stack top value                                                                                                                                 |
| `MUL`  | 0x10 | 0     | self-explanatory                                                                                                                                                           |
| `DIV`  | 0x11 | 0     | idem. yields a division by zero error if the stack top value is an integer 0                                                                                               |
| `MOD`  | 0x12 | 0     |                                                                                                                                                                            |
| `POW`  | 0x13 | 0     |                                                                                                                                                                            |

### C. The virtual machine

The virtual machine's stack can store a maximum of 0xFFFF elements.

It runs indefinitely until it encounters the `HLT` instruction. In
general, there are no guards against invalid code in the virtual
machine (as all the code it executes is supposed to have been
generated by the compiler).

Values are represented using a 72-byte structure as follows:

```c
struct memory_object {
	enum memory_object_type	type;
	union {
		int32_t		i;
		double		d;
	}			value;
}
```

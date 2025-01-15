#ifndef COMPILER_H_
# define COMPILER_H_

# include <parser.h>

# define MAX_BYTECODE_SIZE (0xFFFF)

/* numbers are LITTLE ENDIAN */

enum opcode : uint8_t
{
    HLT = 0x00, /* halt the execution */
    INT = 0x01, /* push the following integer onto the stack - 4 */
    FLT = 0x02, /* push the following float onto the stack - 8 */
    STR = 0x03, /* store the top of stack value into the following variable - 4 */
    LOD = 0x04, /* load the following variable on top of the stack - 4 */
    PRM = 0x05, /* call the following primitive with the top values of the stack - 4 */
    JMP = 0x06, /* jump to the following address - 4 */
    JPI = 0x07, /* jump to the following adress if the top of the stack is non-zero - 4 */
    AND = 0x08, /* perform the logical & of the stack top values */
    OOR = 0x09, /*   "      "     "    | "   "    "    "    " */
    NOT = 0x0A, /*   "      "     "    ~ "   "    "    "    " */
    CEQ = 0x0B, /* check if the stack top values are equal */
    ORD = 0x0C, /* check the order of the stack top values : -1 if
                   the top is less than the next, 0 if equal and 1
                   if the top greater than the next */
    ADD = 0x0D, /* self-explanatory */
    SUB = 0x0E, /* idem */
    NEG = 0x0F,
    MUL = 0x10,
    DIV = 0x11,
    MOD = 0x12,
    POW = 0x13,
    RDV = 0x14, /* read standard input into the following variable - 4 */
};

struct bytecode {
    struct dynarray	*vars;

    enum opcode		bytes[MAX_BYTECODE_SIZE];
    size_t		byte_c;
};

struct bytecode	init_bytecode (struct dynarray *);
int		compile (struct expr *, struct bytecode *);
void		disassembly (struct bytecode *);

#endif /* COMPILER_H_ */

#include <compiler.h>
#include <resolver.h>

struct bytecode
init_bytecode (vars)
	struct dynarray *vars;
{
    struct bytecode bc;

    bc.vars = vars;
    bc.byte_c = 0;

    return bc;
}

int
push_byte (bytecode, byte)
	struct bytecode	*bytecode;
	enum opcode	byte;
{
    if (bytecode->byte_c + 1 <= MAX_BYTECODE_SIZE)
    {
        bytecode->bytes[bytecode->byte_c++] = byte;

        return 0;
    }

    return -1;
}

int
push_int (bc, x)
	struct bytecode *bc;
	int32_t		x;
{
    push_byte (bc, x & 0xFF);
    push_byte (bc, (x >> 8) & 0xFF);
    push_byte (bc, (x >> 16) & 0xFF);
    return push_byte (bc, (x >> 24) & 0xFF);
}

int
push_flt (bc, x)
	struct bytecode	*bc;
	double		x;
{
    int64_t y = *(int64_t *)&x;

    push_byte (bc, y & 0xFF);
    push_byte (bc, (y >> 8) & 0xFF);
    push_byte (bc, (y >> 16) & 0xFF);
    push_byte (bc, (y >> 24) & 0xFF);
    push_byte (bc, (y >> 32) & 0xFF);
    push_byte (bc, (y >> 40) & 0xFF);
    push_byte (bc, (y >> 48) & 0xFF);
    return push_byte (bc, (y >> 56) & 0xFF);
}

int
compile (expr, bytecode)
	struct expr	*expr;
	struct bytecode	*bytecode;
{
    size_t i = 0;

    switch (expr->type)
    {
    case EXPR_IDENTIFIER:
        {
            size_t x = find_var (bytecode->vars, expr->value.str);

            push_byte (bytecode, LOD);
            return push_int (bytecode, x);
        }
    case EXPR_BINDING:
        {
            size_t x = find_var (bytecode->vars, ((struct expr *)(expr->children.array[0]))->value.str);

            if (compile ((struct expr *)expr->children.array[1], bytecode))
                return 1;

            push_byte (bytecode, STR);
            return push_int (bytecode, x);
        }
    case EXPR_INTEGER:
        push_byte (bytecode, INT);
        return push_int (bytecode, expr->value.itg);
    case EXPR_FLOAT:
        push_byte (bytecode, FLT);
        return push_flt (bytecode, expr->value.flt);
    case EXPR_PRIMCALL:
        /* children are pushed in the order they appear */
        {
            size_t i;

            for (i = 0; i < expr->children.size; i++)
            {
                struct expr *child = (struct expr *)expr->children.array[i];
                if (compile (child, bytecode))
                    return 1;
            }
            push_byte (bytecode, PRM);
            return push_int (bytecode, expr->value.prim);
        }
    case EXPR_IF:
        {
            int ret;

            size_t jumpi_then, jumpi_else;

            size_t then_begin, else_begin;

            /* condition */
            if (compile ((struct expr *)expr->children.array[0], bytecode))
                return 1;

            jumpi_then = bytecode->byte_c;
            /* 1 byte opcode ; 4 bytes address */
            bytecode->byte_c += 1 + 4;

            jumpi_else = bytecode->byte_c;
            /* 1 byte opcode ; 4 bytes address */
            bytecode->byte_c += 1 + 4;

            then_begin = bytecode->byte_c;

            /* then */
            if (compile ((struct expr *)expr->children.array[1], bytecode))
                return 1;

            else_begin = bytecode->byte_c;

            bytecode->byte_c = jumpi_then;
            push_byte (bytecode, JPI);
            push_int (bytecode, then_begin);

            bytecode->byte_c = jumpi_else;
            push_byte (bytecode, JMP);
            /*
             * if there is an `else' clause, additional space needs
             * to be allocated for the `jump' above it at the end of
             * the `then' clause.  the necessary space is 1 byte opcode
             * and 4 bytes address.
             */
            ret = push_int (bytecode, else_begin + ((expr->children.size == 3) ? 5 : 0));

            bytecode->byte_c = else_begin;

            /* eventual else */
            if (expr->children.size == 3)
            {
                size_t else_end;

                /* 1 byte opcode ; 4 bytes address */
                bytecode->byte_c += 1 + 4;

                compile ((struct expr *)expr->children.array[2], bytecode);

                else_end = bytecode->byte_c;

                bytecode->byte_c = else_begin;

                push_byte (bytecode, JMP);
                ret = push_int (bytecode, else_end);
                bytecode->byte_c = else_end;
            }

            return ret;
        }
    case EXPR_WHILE:
        {
            size_t cond_start, jumpi_do, jumpi_break, do_start, after_start;

            cond_start = bytecode->byte_c;

            /* cond */
            if (compile ((struct expr *)expr->children.array[0], bytecode))
                return 1;

            jumpi_do = bytecode->byte_c;
            /* 1 byte opcode ; 4 bytes address */
            bytecode->byte_c += 1 + 4;

            jumpi_break = bytecode->byte_c;
            /* 1 byte opcode ; 4 bytes address */
            bytecode->byte_c += 1 + 4;

            do_start = bytecode->byte_c;

            if (compile ((struct expr *)expr->children.array[1], bytecode))
                return 1;

            /* after the jump to the condition */
            after_start = bytecode->byte_c;

            bytecode->byte_c = jumpi_do;
            push_byte (bytecode, JPI);
            push_int (bytecode, do_start);

            bytecode->byte_c = jumpi_break;
            push_byte (bytecode, JMP);
            push_int (bytecode, after_start + 5);

            bytecode->byte_c = after_start;

            push_byte (bytecode, JMP);
            return push_int (bytecode, cond_start);
        }
    case EXPR_OP:
        {
            size_t i;

            for (i = 0; i < expr->children.size; i++)
            {
                if (compile ((struct expr *)expr->children.array[i], bytecode))
                    return 1;
            }

            switch (expr->value.op)
            {
            case PLUS:
                return push_byte (bytecode, ADD);
            case MINUS:
                if (expr->children.size == 1)
                    return push_byte (bytecode, NEG);
                else
                    return push_byte (bytecode, SUB);
            case TIMES:
                return push_byte (bytecode, MUL);
            case DIVISION:
                return push_byte (bytecode, DIV);
            case POWER:
                return push_byte (bytecode, POW);
            case MODULO:
                return push_byte (bytecode, MOD);

            case EQ:
                return push_byte (bytecode, CEQ);
            case NEQ:
                push_byte (bytecode, CEQ);
                return push_byte (bytecode, NOT);
            case GT:
                push_byte (bytecode, ORD);
                push_byte (bytecode, INT);
                push_int (bytecode, 1);
                return push_byte (bytecode, CEQ);
            case LT:
                push_byte (bytecode, ORD);
                push_byte (bytecode, INT);
                push_int (bytecode, -1);
                return push_byte (bytecode, CEQ);
            case GE:
                push_byte (bytecode, ORD);
                push_byte (bytecode, INT);
                push_int (bytecode, -1);
                push_byte (bytecode, CEQ);
                return push_byte (bytecode, NOT);
            case LE:
                push_byte (bytecode, ORD);
                push_byte (bytecode, INT);
                push_int (bytecode, 1);
                push_byte (bytecode, CEQ);
                return push_byte (bytecode, NOT);

            case OP_AND:
                return push_byte (bytecode, AND);
            case OR:
                return push_byte (bytecode, OOR);
            case OP_NOT:
                return push_byte (bytecode, NOT);
            }
        }
    case EXPR_PROGRAM:
        break;
    }

   for (i = 0; i < expr->children.size; i++)
   {
       if (compile ((struct expr *)expr->children.array[i], bytecode))
           return 1;
   }

   return 0;
}

static const char *const __opcode_names[] =
{ "HLT", "INT", "FLT", "STR", "LOD", "PRM", "JMP", "JPI", "AND", "OOR",
  "NOT", "CEQ", "ORD", "ADD", "SUB", "NEG", "MUL", "DIV", "MOD", "POW" };

static inline size_t
__print_next (bc, n, s)
	struct bytecode	*bc;
	size_t		n, s;
{
    size_t i;

    for (i = 0; i < n;)
    {
        printf (" %02x", bc->bytes[s + ++i]);
    }

    return n;
}

void
disassembly (bc)
	struct bytecode *bc;
{
    size_t i;

    for (i = 0; i < bc->byte_c; i++)
    {
       enum opcode current = bc->bytes[i];

       printf ("%04lx\t%s", i, __opcode_names[current]);

       switch (current)
       {
       case FLT:
           i += __print_next (bc, 8, i);
           break;
       case INT:
       case STR:
       case LOD:
       case PRM:
       case JMP:
       case JPI:
           i += __print_next (bc, 4, i);
           break;
       default:
           break;
       }

       putchar ('\n');
    }
}

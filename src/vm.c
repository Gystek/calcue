#include <arith.h>
#include <errno.h>
#include <lexer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vm.h>

struct vm *
init_vm (bc, vm)
	struct bytecode *bc;
	struct vm	*vm;
{
    size_t i;

    vm->pc = 0;
    vm->sp = 0;

    vm->var_c = bc->vars->size;
    vm->vars = (char **)bc->vars->array;

    vm->environment = malloc (sizeof(struct memory_object) * vm->var_c);

    if (!vm->environment)
        return NULL;

    for (i = 0; i < bc->byte_c; i++)
    {
        vm->memory[i] = bc->bytes[i];
    }

    return vm;
}

static inline enum opcode
read (vm)
	struct vm *vm;
{
    return vm->memory[vm->pc++];
}

static inline void
stack_push (vm, obj)
	struct vm		*vm;
	struct memory_object	obj;
{
    vm->stack[vm->sp++] = obj;
}

static inline struct memory_object
stack_pop (vm)
	struct vm *vm;
{
    return vm->stack[--vm->sp];
}

static inline int32_t
read4 (vm)
	struct vm *vm;
{
    int32_t lsb = (int32_t)read (vm);
    int32_t b1  = (int32_t)read (vm);
    int32_t b2  = (int32_t)read (vm);
    int32_t msb = (int32_t)read (vm);

    return (lsb | (b1 << 8) | (b2 << 16) | (msb << 24));
}

static inline int64_t
read8 (vm)
	struct vm *vm;
{
    int64_t lsb = (int64_t)read (vm);
    int64_t b1  = (int64_t)read (vm);
    int64_t b2  = (int64_t)read (vm);
    int64_t b3  = (int64_t)read (vm);
    int64_t b4  = (int64_t)read (vm);
    int64_t b5  = (int64_t)read (vm);
    int64_t b6  = (int64_t)read (vm);
    int64_t msb = (int64_t)read (vm);

    return (lsb
            | (b1 << 8)
            | (b2 << 16)
            | (b3 << 24)
            | (b4 << 32)
            | (b5 << 40)
            | (b6 << 48)
            | (msb << 56));
}

static inline struct memory_object
make_mem_i32 (i)
	int32_t i;
{
    struct memory_object obj;

    obj.type = M_I32;
    obj.value.i = i;

    return obj;
}

static inline struct memory_object
make_mem_f64 (d)
	double d;
{
    struct memory_object obj;

    obj.type = M_F64;
    obj.value.d = d;

    return obj;
}

static inline bool
mem_to_bool (obj)
	struct memory_object obj;
{
    if (obj.type == M_I32 && obj.value.i == 0)
        return 0;
    if (obj.type == M_F64 && obj.value.d == 0.0)
        return 0;

    return 1;
}

static inline struct memory_object
bool_to_mem (b, type)
	bool			b;
	enum memory_object_type	type;
{
    struct memory_object obj;
    obj.type = type;

    switch (type)
    {
    case M_I32:
        obj.value.i = (int32_t)b;
        break;
    case M_F64:
        obj.value.d = (double)b;
        break;
    }

    return obj;
}

static void
print_object (obj)
	struct memory_object obj;
{
    if (obj.type == M_I32)
        printf ("%d", obj.value.i);
    else
        printf ("%ff", obj.value.d);
}

static int
__primitive_print (vm)
	struct vm *vm;
{
    struct memory_object obj = stack_pop (vm);

    print_object (obj);

    putchar ('\n');

    return 0;
}

static int
__primitive_dump (vm)
	struct vm *vm;
{
    size_t i;

    for (i = 0; i < vm->var_c; i++)
    {
        printf ("%s = ", vm->vars[i]);
        print_object (vm->environment[i]);
        printf (" ; ");
    }

    putchar ('\n');

    return 0;
}

static int
__primitive_log (vm)
	struct vm *vm;
{
    double x;
    struct memory_object obj = stack_pop (vm);

    if (obj.type == M_I32)
        x = (double)obj.value.i;
    else
        x = obj.value.d;

    stack_push (vm, make_mem_f64 (log (x)));

    return 0;
}

static int
__primitive_exp (vm)
	struct vm *vm;
{
    double x;
    struct memory_object obj = stack_pop (vm);

    if (obj.type == M_I32)
        x = (double)obj.value.i;
    else
        x = obj.value.d;

    stack_push (vm, make_mem_f64 (exp (x)));

    return 0;

}

static int
__primitive_sqrt (vm)
	struct vm *vm;
{
    double x;
    struct memory_object obj = stack_pop (vm);

    if (obj.type == M_I32)
        x = (double)obj.value.i;
    else
        x = obj.value.d;

    stack_push (vm, make_mem_f64 (sqrt (x)));

    return 0;

}

static int (*__primitives[KW_N - PRIMS_START])(struct vm *) =
{
    __primitive_print,
    __primitive_dump,
    __primitive_log,
    __primitive_exp,
    __primitive_sqrt
};

#define __ORD(x, y) ({\
                      int ret = 0;\
                      if (x < y)\
                        ret = -1;\
                      else if (x > y)\
                        ret = 1;\
                      ret;\
})

static struct memory_object
__mem_and (lhs, rhs)
	struct memory_object lhs, rhs;
{
    bool lhb, rhb;

    lhb = mem_to_bool (lhs);
    rhb = mem_to_bool (rhs);

    return bool_to_mem (lhb && rhb, M_I32);
}

static struct memory_object
__mem_or (lhs, rhs)
	struct memory_object lhs, rhs;
{
    bool lhb, rhb;

    lhb = mem_to_bool (lhs);
    rhb = mem_to_bool (rhs);

    return bool_to_mem (lhb || rhb, M_I32);
}

static struct memory_object
__mem_add (lhs, rhs)
	struct memory_object lhs, rhs;
{
    struct memory_object obj;

    obj.type = M_F64;
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        obj.type = M_I32;
        obj.value.i = lhs.value.i + rhs.value.i;
    }
    else if (lhs.type == M_F64 && rhs.type == M_I32)
        obj.value.d = lhs.value.d + rhs.value.i;
    else if (lhs.type == M_I32 && rhs.type == M_F64)
        obj.value.d = lhs.value.i + rhs.value.d;
    else
        obj.value.d = lhs.value.d + rhs.value.d;

    return obj;
}

static struct memory_object
__mem_sub (lhs, rhs)
	struct memory_object lhs, rhs;
{
    struct memory_object obj;

    obj.type = M_F64;
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        obj.type = M_I32;
        obj.value.i = lhs.value.i - rhs.value.i;
    }
    else if (lhs.type == M_F64 && rhs.type == M_I32)
        obj.value.d = lhs.value.d - rhs.value.i;
    else if (lhs.type == M_I32 && rhs.type == M_F64)
        obj.value.d = lhs.value.i - rhs.value.d;
    else
        obj.value.d = lhs.value.d - rhs.value.d;

    return obj;
}

static struct memory_object
__mem_mul (lhs, rhs)
	struct memory_object lhs, rhs;
{
    struct memory_object obj;

    obj.type = M_F64;
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        obj.type = M_I32;
        obj.value.i = lhs.value.i * rhs.value.i;
    }
    else if (lhs.type == M_F64 && rhs.type == M_I32)
        obj.value.d = lhs.value.d * rhs.value.i;
    else if (lhs.type == M_I32 && rhs.type == M_F64)
        obj.value.d = lhs.value.i * rhs.value.d;
    else
        obj.value.d = lhs.value.d * rhs.value.d;

    return obj;
}

static struct memory_object
__mem_div (lhs, rhs)
	struct memory_object lhs, rhs;
{
    struct memory_object obj;

    obj.type = M_F64;
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        obj.type = M_I32;
        obj.value.i = lhs.value.i / rhs.value.i;
    }
    else if (lhs.type == M_F64 && rhs.type == M_I32)
        obj.value.d = lhs.value.d / (double)rhs.value.i;
    else if (lhs.type == M_I32 && rhs.type == M_F64)
        obj.value.d = (double)lhs.value.i / rhs.value.d;
    else
        obj.value.d = lhs.value.d / rhs.value.d;

    return obj;
}

static struct memory_object
__mem_mod (lhs, rhs)
	struct memory_object lhs, rhs;
{
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        struct memory_object obj;

        obj.type = M_I32;
        obj.value.i = lhs.value.i % rhs.value.i;

        return obj;
    }

    return lhs;
}

static struct memory_object
__mem_pow (lhs, rhs)
	struct memory_object lhs, rhs;
{
    struct memory_object obj;

    obj.type = M_F64;
    if (lhs.type == M_I32 && rhs.type == M_I32)
    {
        obj.type = M_I32;
        obj.value.i = powi (lhs.value.i, rhs.value.i);
    }
    else if (lhs.type == M_F64 && rhs.type == M_I32)
        obj.value.d = pow (lhs.value.d, (double)rhs.value.i);
    else if (lhs.type == M_I32 && rhs.type == M_F64)
        obj.value.d = pow ((double)lhs.value.i, rhs.value.d);
    else
        obj.value.d = pow (lhs.value.d, rhs.value.d);

    return obj;
}

/*
 * 0  - no error
 * -1 - error
 * 1  - HLT
 */
static int
cycle (vm)
	struct vm *vm;
{
    enum opcode instr = read (vm);

    #ifdef _DUMP_VM_EVERY_CYCLE
    	printf ("op = %x\n", instr);
    #endif

    switch (instr)
    {
    case HLT:
        vm->sp = 0;
        return 1;
    case INT:
        stack_push (vm, make_mem_i32 (read4 (vm)));
        break;
    case FLT:
        stack_push (vm, make_mem_f64 (read8(vm)));
        break;
    case STR:
        {
            uint32_t var = (uint32_t)read4 (vm);

            vm->environment[var] = stack_pop (vm);
        }
        break;
    case LOD:
        {
            uint32_t var = (uint32_t)read4 (vm);

            stack_push (vm, vm->environment[var]);
        }
        break;
    case PRM:
        {
            uint32_t prim = (uint32_t)read4 (vm);

            return __primitives[prim] (vm);
        }
    case JMP:
        {
            uint16_t addr = (uint16_t)read4 (vm);

            vm->pc = addr;
        }
        break;
    case JPI:
        {
            uint16_t addr = (uint16_t)read4 (vm);

            if (mem_to_bool (stack_pop (vm)))
                vm->pc = addr;
        }
        break;
    case AND:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_and (lhs, rhs));
        }
        break;
     case OOR:
         {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_or (lhs, rhs));
         }
         break;
     case NOT:
         {
             struct memory_object obj = stack_pop (vm);

             stack_push (vm, bool_to_mem (!mem_to_bool (obj), obj.type));
         }
         break;
    case CEQ:
        {
            struct memory_object rhs, lhs;
            bool b;

            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            if (lhs.type == M_I32)
            {
                if (rhs.type == M_I32)
                    b = lhs.value.i == rhs.value.i;
                else
                    b = false;
            }
            else
            {
                if (rhs.type == M_F64)
                    b = lhs.value.d == rhs.value.d;
                else
                    b = false;
            }

            stack_push (vm, bool_to_mem (b, M_I32));
        }
        break;
    case ORD:
        {
            struct memory_object rhs, lhs, obj;
            int ord;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            obj.type = M_I32;

            if (lhs.type == M_I32 && rhs.type == M_I32)
                ord = __ORD(lhs.value.i, rhs.value.i);
            else if (lhs.type == M_F64 && rhs.type == M_I32)
                ord = __ORD(lhs.value.d, rhs.value.i);
            else if (lhs.type == M_I32 && rhs.type == M_F64)
                ord = __ORD(lhs.value.i, rhs.value.d);
            else
                ord = __ORD(lhs.value.d, rhs.value.d);

            obj.value.i = ord;

            stack_push (vm, obj);
        }
        break;
    case NEG:
        {
            struct memory_object obj = stack_pop (vm);

            if (obj.type == M_I32)
                obj.value.i = -obj.value.i;
            else
                obj.value.d = -obj.value.d;

            stack_push (vm, obj);
        }
    case ADD:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_add (lhs, rhs));
         }
         break;
    case SUB:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_sub (lhs, rhs));
         }
         break;
    case MUL:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_mul (lhs, rhs));
         }
         break;
    case DIV:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_div (lhs, rhs));
         }
         break;
    case MOD:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_mod (lhs, rhs));
         }
         break;
    case POW:
        {
            struct memory_object rhs, lhs;
            rhs = stack_pop (vm);
            lhs = stack_pop (vm);

            stack_push (vm, __mem_pow (lhs, rhs));
         }
         break;
    case RDV:
        {
            char buffer[MAX_NUMBER_LEX + 1] = { 0 }, *iend = NULL;
            int32_t i;
            double d;

            struct memory_object obj;

            uint32_t var = (uint32_t)read4 (vm);

            if (!fgets (buffer, MAX_NUMBER_LEX, stdin))
            {
                fprintf (stderr, "error: failed to read keyboard\n");

                return -1;
            }

            /* remove trailing \n */
            buffer[strlen (buffer) - 1] = 0;

            errno = 0;
            i = strtol (buffer, &iend, 10);

            if (errno != 0 || iend != buffer + strlen (buffer))
            {
                errno = 0;
                d = strtod (buffer, &iend);

                if (errno != 0 || iend != buffer + strlen (buffer))
                {
                    fprintf (stderr, "error: `%s' is not a valid number\n",
                                     buffer);

                    return -1;
                }

                obj = make_mem_f64 (d);
            }
            else
                obj = make_mem_i32 (i);

            vm->environment[var] = obj;
        }
        break;
    }

    #ifdef _DUMP_VM_EVERY_CYCLE

    dump_vm (*vm);

    #endif

    return 0;
}

int
run_vm (vm)
	struct vm *vm;
{
    for (;;)
    {
        int ret = cycle (vm);

        switch (ret)
        {
        case 0:
            break;
        case 1:
            /* HLT */
            return 0;
        default:
            return ret;
        }
    }
}

void
dump_vm (vm)
	struct vm vm;
{
    size_t i;

    printf ("pc = %04x\n", vm.pc);
    printf ("sp = %04lx\n", vm.sp);

    printf ("stack = [");
    for (i = 0; i < vm.sp; i++)
    {
        if (i != 0)
            putchar (' ');

        print_object (vm.stack[i]);
    }
    puts ("]");

    printf ("vars: ");

    __primitive_dump (&vm);
}

void
destroy_vm (vm)
	struct vm vm;
{
    free (vm.environment);
}

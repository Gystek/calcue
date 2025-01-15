#ifndef VM_H_
# define VM_H_

# include <compiler.h>
# include <stddef.h>
# include <stdint.h>

enum memory_object_type {
    M_I32,
    M_F64
};

struct memory_object {
    enum memory_object_type	type;
    union {
        int32_t			i;
        double			d;
    }				value;
};

# define MEMORY_SIZE (0xFFFF)
# define STACK_SIZE (0xFFFF)

struct vm {
    struct memory_object	stack[STACK_SIZE];
    uint8_t			memory[MEMORY_SIZE];
    struct memory_object	*environment;

    uint16_t			pc;
    size_t			sp;

    char			**vars;
    size_t			var_c;
};

struct vm	*init_vm (struct bytecode *, struct vm *vm);
int		run_vm (struct vm *);
void		dump_vm (struct vm);
void		destroy_vm (struct vm);

#endif /* VM_H_ */

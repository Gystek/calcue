#ifndef DYNARRAY_H_
# define DYNARRAY_H_

# include <stddef.h>
# include <stdint.h>

struct dynarray {
    intptr_t	*array;
    size_t	capacity;
    size_t	size;
};

struct dynarray	new_dynarray (void);
struct dynarray new_dynarray_with_capacity (size_t);
int		push_dynarray (struct dynarray *, intptr_t);
intptr_t	pop_dynarray (struct dynarray *);

void		destroy_dynarray (struct dynarray);

#endif /* DYNARRAY_H_ */

#ifndef RESOLVER_H_
# define RESOLVER_H_

# include <dynarray.h>

/* The resolver, well, *resolves* the variable names,
 * turns them into indices and makes sure that no undefined
 * variables are used.
 *
 * If undefined variables are used, an error message is printed
 * and -1 is returned.
 */
int	resolve (struct expr *, struct dynarray *);

size_t	find_var (struct dynarray *, char *);

#endif /* RESOLVER_H_ */

#ifndef LIST_H_
# define LIST_H_

# include <stddef.h>
# include <stdint.h>

struct list_node {
    intptr_t		val;
    struct list_node	*next;
};

struct list {
    struct list_node	*head;
    size_t		length;
};

struct list	new_list (void);
int		push_list (struct list *, intptr_t);
intptr_t	pop_list (struct list *);
size_t		list_len (struct list *);

int		destroy_list (struct list *);

#endif /* LIST_H_ */

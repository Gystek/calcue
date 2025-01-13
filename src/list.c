#include <list.h>
#include <stdlib.h>

struct list
new_list (void)
{
    struct list list;

    list.head = NULL;
    list.length = 0;

    return list;
}

int
push_list (list, val)
	struct list	*list;
	intptr_t	val;
{
    struct list_node *new_head;

    if (!list)
        return 1;

    new_head = malloc (sizeof(struct list_node));

    if (!new_head)
        return 1;

    new_head->val = val;
    new_head->next = list->head;

    list->head = new_head;
    list->length += 1;

    return 0;
}

intptr_t
pop_list (list)
	struct list *list;
{
    struct list_node	*old_head;
    intptr_t		old_val;

    if (!list || list->length <= 0)
        return 0;

    old_head = list->head;
    old_val = old_head->val;

    list->head = old_head->next;
    list->length -= 1;

    free (old_head);

    return old_val;
}

size_t
list_len (list)
	struct list *list;
{
    if (!list)
        return 0;

    return list->length;
}

int
destroy_list (list)
	struct list *list;
{
    while (pop_list (list))
        ;

    return 0;
}

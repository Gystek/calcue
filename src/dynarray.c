#include <dynarray.h>
#include <stdlib.h>

struct dynarray
new_dynarray (void)
{
    struct dynarray dynarray;

    dynarray.array = NULL;
    dynarray.capacity = 0;
    dynarray.size = 0;

    return dynarray;
}

int
push_dynarray (array, val)
	struct dynarray	*array;
	intptr_t	val;
{
    if (array->size + 1 > array->capacity)
    {
        array->capacity = 1 + 2 * array->capacity;
        intptr_t *narray = realloc (array->array,
                                    sizeof(intptr_t) * array->capacity);

        if (!narray)
            return 1;

        array->array = narray;
    }

    array->array[array->size++] = val;

    return 0;
}

intptr_t
pop_dynarray (array)
	struct dynarray *array;
{
    if (array->size <= 0)
        return 0;

    return array->array[--array->size];
}

void
destroy_dynarray (array)
	struct dynarray *array;
{
    free (array->array);
}

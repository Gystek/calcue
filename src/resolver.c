#include <parser.h>
#include <resolver.h>
#include <string.h>

size_t
find_var (vars, name)
	struct dynarray	*vars;
	char		*name;
{
    size_t i = 0;

    for (i = 0; i < vars->size; i++)
    {
        char *other = (char *)vars->array[i];

        if (strcmp (other, name) == 0)
            break;
    }

    return i;
}

int
resolve (expr, vars)
	struct expr	*expr;
	struct dynarray	*vars;
{
    size_t i;
    int t = 0;

    switch (expr->type)
    {
    case EXPR_READ:
        {
            char *name = expr->value.str;

            if (find_var (vars, name) >= vars->size)
                push_dynarray (vars, (intptr_t)name);
        }
        return 0;
    case EXPR_BINDING:
        {
            char *name = ((struct expr *)(expr->children.array[0]))->value.str;

            if (find_var (vars, name) >= vars->size)
                push_dynarray (vars, (intptr_t)name);
        }
        break;
    case EXPR_IDENTIFIER:
        {
            char *name = expr->value.str;

            if (find_var (vars, name) >= vars->size)
            {
                fprintf (stderr, "error: name `%s' isn't defined\n", name);

                return -1;
            }
        }
    default:
        break;
    }

    for (i = 0; i < expr->children.size; i++)
    {
        t += resolve ((struct expr *)expr->children.array[i], vars);
    }

    if (t)
        return -1;

    return 0;
}

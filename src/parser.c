#include <dynarray.h>
#include <parser.h>
#include <stdlib.h>
#include <string.h>

struct parser
init_parser (input, input_n)
	struct lexeme	**input;
	size_t		input_n;
{
    struct parser parser;

    parser.input = input;
    parser.input_n = input_n;
    parser.cur = 0;

    return parser;
}

static struct lexeme **
lookahead (parser, n)
	struct parser	*parser;
	size_t		n;
{
    if (parser->cur + n >= parser->input_n)
        return NULL;

    return (parser->input + parser->cur + n);
}

static struct lexeme
consume (parser)
	struct parser *parser;
{
    return *parser->input[parser->cur++];
}

#define __destroy_dynarray_exprs(x) {\
    size_t i = 0;\
    for (i = 0; i < x.size; i++)\
        destroy_expr ((struct expr *)x.array[i]);\
    destroy_dynarray (x);\
}\

#define IS_KEYWORD(x, s) ((*x)->type == KEYWORD\
                          && strcmp ((*x)->value.kw, s) == 0)

static struct expr *parse_one (struct parser *);
static struct expr *parse_expr (struct parser *);

#define ARE_OPS(p, ...) ({\
    enum operator ops[] = { __VA_ARGS__ };\
    size_t        opn = sizeof(ops)/sizeof(enum operator);\
    int ret = 0;\
    struct lexeme **lex = lookahead (p, 0);\
    if (lex && (*lex)->type == OPERATOR)\
    {\
      size_t i;\
      for (i = 0; i < opn; i++)\
      {\
        if (ops[i] == (*lex)->value.op)\
        {\
          ret = 1;\
        }\
      }\
    }\
    ret;\
})

#define GENERATE_OP_PARSER(name, next_lvl, ...) \
static struct expr * \
name (parser) \
	struct parser *parser;\
{\
   struct expr *lhs = NULL, *rhs = NULL;\
   lhs = next_lvl(parser);\
   if (!lhs)\
       return NULL;\
   while (ARE_OPS(parser, __VA_ARGS__))\
   {\
       struct expr *new = NULL;\
       struct lexeme op = consume (parser);\
       rhs = next_lvl (parser);\
       if (!rhs)\
           goto _CLEANUP_EXPRS;\
       new = new_expr (EXPR_OP);\
       if (!new)\
           goto _CLEANUP_EXPRS;\
       new->value.op = op.value.op;\
       new->children = new_dynarray ();\
       push_dynarray (&new->children, (intptr_t)lhs);\
       push_dynarray (&new->children, (intptr_t)rhs);\
       lhs = new;\
   }\
   return lhs;\
_CLEANUP_EXPRS:\
   destroy_expr (lhs);\
   destroy_expr (rhs);\
   return NULL;\
}

static struct expr *
parse_identifier (parser)
	struct parser *parser;
{
    char		*id;
    struct expr		*expr;
    struct lexeme	**next = lookahead (parser, 0);

    if (!next)
    {
        fprintf (stderr, "input: expected identifier, found end of file\n");

        return NULL;
    }

    if ((*next)->type != IDENTIFIER)
    {
        __lexer_perror(*next, "expected identifier\n");

        return NULL;
    }

    id = consume (parser).value.str;
    expr = new_expr (EXPR_IDENTIFIER);

    if (!expr)
        return NULL;

    expr->value.str = id;

    return expr;
}

/* GSBNF : Gustek's Shady Backus-Naur Form
 *
 * program   ::= (statement (SEPARATOR+ statement)*)?		--
 * statement ::= binding | if-then | while-do | expr		--
 * binding   ::= IDENTIFIER BINDING expr			--
 * if-then   ::= IF expr THEN program (ELSE program)? END	--
 * while-do  ::= WHILE expr DO program END			--
 * expr      ::= logop ((AND | OR) logop)*			--
 * logop     ::= eqneq ((EQ | NEQ) eqneq)*			--
 * eqneq     ::= cmp ((GT | LT | GE | LE) cmp)*			--
 * cmp       ::= sum ((PLUS | MINUS) sum)*			--
 * sum       ::= mul ((MUL | DIV | MOD) mul)*                   --
 * mul       ::= pow (POWER pow)*				--
 * pow       ::= primcall expr+ | unary				--
 * unary     ::= (MINUS | NOT)? literal				--
 * literal   ::= INTEGER | FLOAT | IDENTIFIER | L_PAR expr R_PAR --
 */
static struct expr *
parse_literal (parser)
	struct parser *parser;
{
    struct expr *lit = NULL;
    struct lexeme **next = lookahead (parser, 0);

    if (!next)
        fprintf (stderr, "expected number, identifier or parenthesised expression, found end of file\n");

    if ((*next)->type == L_PAR)
    {
        /* left paren */
        consume (parser);

        lit = parse_expr (parser);

        if (!lit)
            return NULL;

        next = lookahead (parser, 0);
        if (!next)
        {
            fprintf (stderr, "expected closing parenthesis, found end of file\n");
            return NULL;
        }

        if ((*next)->type == R_PAR)
            consume (parser);
        else
        {
            __lexer_perror(*next, "expected closing parenthesis\n");

            return NULL;
        }

        return lit;
    }


    if ((*next)->type == IDENTIFIER)
        return parse_identifier (parser);


    lit = new_expr (EXPR_INTEGER);

    if (!lit)
        return NULL;


    if ((*next)->type == INTEGER)
    {
        consume (parser);
        lit->value.itg = (*next)->value.itg;
    }
    else if ((*next)->type == FLOAT)
    {
        consume (parser);
        lit->type = EXPR_FLOAT;
        lit->value.flt = (*next)->value.flt;
    }
    else
    {
        __lexer_perror(*next, "expected number, identifier or parenthesised expression\n");

        return NULL;
    }

    return lit;
}

static struct expr *
parse_unary (parser)
	struct parser *parser;
{
    if (ARE_OPS(parser, MINUS, NOT))
    {
        struct expr  *operand, *unary;
        struct lexeme op = consume (parser);


        operand = parse_unary (parser);

        if (!operand)
            return NULL;

        unary = new_expr (EXPR_OP);

        if (!unary)
        {
            destroy_expr (operand);

            return NULL;
        }

        unary->value.op = op.value.op;
        unary->children = new_dynarray ();

        push_dynarray (&unary->children, (intptr_t)operand);

        return unary;
    }


    return parse_literal (parser);
}

static struct expr *
parse_pow (parser)
	struct parser *parser;
{
    struct lexeme **next = lookahead (parser, 0);

    if (next && (*next)->type == KEYWORD)
    {
        /* Cf. the note on this at include/lexer.h */
        size_t i = PRIMS_START;


        for (; i < KW_N; i++)
        {
            if (strcmp ((*next)->value.kw, keywords[i]) == 0)
            {
                /* A primitive has been found, we proceed with it */
                size_t prim_arity = prim_arities[i - PRIMS_START];
                size_t j;


                struct expr *prim = new_expr (EXPR_PRIMCALL);

                if (!prim)
                    return NULL;

                prim->value.prim = i - PRIMS_START;
                prim->children = new_dynarray ();

                /* primitive */
                consume (parser);

                for (j = 0; j < prim_arity; j++)
                {
                    struct expr *arg = parse_expr (parser);

                    if (!arg)
                    {
                        __destroy_dynarray_exprs(prim->children);
                        destroy_expr (prim);

                        return NULL;
                    }

                    push_dynarray (&prim->children, (intptr_t)arg);
                }

                return prim;
            }
        }
    }


    return parse_unary (parser);
}

GENERATE_OP_PARSER(parse_mul, parse_pow, POWER)
GENERATE_OP_PARSER(parse_sum, parse_mul, TIMES, DIVISION, MODULO)
GENERATE_OP_PARSER(parse_cmp, parse_sum, PLUS, MINUS)
GENERATE_OP_PARSER(parse_eqneq, parse_cmp, GT, LT, GE, LE)
GENERATE_OP_PARSER(parse_logop, parse_eqneq, EQ, NEQ)
GENERATE_OP_PARSER(parse_expr, parse_logop, AND, OR)

static struct expr *
parse_binding (parser)
	struct parser *parser;
{
    struct expr *id = NULL, *value = NULL, *binding = NULL;

    /* assured to succeed */
    id = parse_identifier (parser);

    /* binding */
    consume (parser);

    value = parse_expr (parser);

    if (!value)
        goto _CLEANUP_EXPRS;

    binding = new_expr (EXPR_BINDING);

    if (!binding)
        goto _CLEANUP_EXPRS;

    binding->children = new_dynarray_with_capacity (2);

    if (!binding->children.capacity)
        goto _CLEANUP_EXPRS;

    push_dynarray (&binding->children, (intptr_t)id);
    push_dynarray (&binding->children, (intptr_t)value);

    return binding;

_CLEANUP_EXPRS:
    destroy_expr (id);
    destroy_expr (value);
    destroy_expr (binding);

    return NULL;
}

struct expr *
parse_program (parser)
	struct parser *parser;
{
    struct expr *prg = NULL;
    struct lexeme **kwtemp = NULL;

    prg = new_expr (EXPR_PROGRAM);

    if (!prg)
        return NULL;

    prg->children = new_dynarray ();

    do
    {
        struct expr *stm = parse_one (parser);

        if (!stm)
        {
            __destroy_dynarray_exprs(prg->children);
            destroy_expr (prg);

            return NULL;
        }

        push_dynarray (&prg->children, (intptr_t)stm);

        kwtemp = lookahead (parser, 0);

        while (kwtemp && (*kwtemp)->type == SEPARATOR)
        {
            /* separator */
            consume (parser);
            kwtemp = lookahead (parser, 0);
        }

        kwtemp = lookahead (parser, 0);
    } while (kwtemp);

    return prg;
}

static struct expr *
parse_program_until_kw (parser, kw)
	struct parser	*parser;
	const char	*kw;
{
    struct expr *prg = NULL;
    struct lexeme **kwtemp = NULL;

    prg = new_expr (EXPR_PROGRAM);

    if (!prg)
        return NULL;

    prg->children = new_dynarray ();

    do
    {
        struct expr *stm = parse_one (parser);

        if (!stm)
        {
            __destroy_dynarray_exprs(prg->children);
            destroy_expr (prg);

            return NULL;
        }

        push_dynarray (&prg->children, (intptr_t)stm);

        kwtemp = lookahead (parser, 0);

        while (kwtemp && (*kwtemp)->type == SEPARATOR)
        {
            /* separator */
            consume (parser);
            kwtemp = lookahead (parser, 0);
        }

        kwtemp = lookahead (parser, 0);
    } while (kwtemp
             && !IS_KEYWORD(kwtemp, kw));

    if (!kwtemp)
    {
        fprintf (stderr, "input: expected `%s', found end of file\n", kw);

        __destroy_dynarray_exprs(prg->children);
        destroy_expr (prg);

        return NULL;
    }

    /* keyword */
    consume (parser);

    return prg;
}

static struct expr *
parse_if_then (parser)
	struct parser *parser;
{
    struct expr *cond = NULL, *then = NULL, *elsethen = NULL,
                *ifthenelse = NULL;

    struct lexeme **kwtemp;

    /* if - assured to succeed */
    consume (parser);

    cond = parse_expr (parser);

    if (!cond)
        goto _CLEANUP_EXPRS;

    kwtemp = lookahead (parser, 0);

    if (!kwtemp)
        fprintf (stderr, "input: expected keyword `then', found end of file\n");
    else if (!IS_KEYWORD(kwtemp, "then"))
        __lexer_perror(*kwtemp, "expected keyword `then'\n");

    /* then */
    consume (parser);

    then = new_expr (EXPR_PROGRAM);

    if (!then)
        goto _CLEANUP_EXPRS;

    then->children = new_dynarray ();

    do
    {
        struct expr *stm = parse_one (parser);

        if (!stm)
        {
            __destroy_dynarray_exprs (then->children);
            goto _CLEANUP_EXPRS;
        }

        push_dynarray (&then->children, (intptr_t)stm);

        kwtemp = lookahead (parser, 0);

        while (kwtemp && (*kwtemp)->type == SEPARATOR)
        {
            /* separator */
            consume (parser);
            kwtemp = lookahead (parser, 0);
        }

        kwtemp = lookahead (parser, 0);
    } while (kwtemp
             && !IS_KEYWORD(kwtemp, "else")
             && !IS_KEYWORD(kwtemp, "end"));

    if (!kwtemp)
    {
        fprintf (stderr, "input: expected `else' or `end', found end of file\n");

        goto _CLEANUP_EXPRS;
    }

    if (IS_KEYWORD(kwtemp, "end"))
    {
        /* end */
        consume (parser);

        ifthenelse = new_expr (EXPR_IF);

        if (!ifthenelse)
        {
            __destroy_dynarray_exprs(then->children);
            goto _CLEANUP_EXPRS;
        }

        ifthenelse->children = new_dynarray_with_capacity (2);

        if (!ifthenelse->children.capacity)
        {
            __destroy_dynarray_exprs(then->children);
            goto _CLEANUP_EXPRS;
        }

        push_dynarray (&ifthenelse->children, (intptr_t)cond);
        push_dynarray (&ifthenelse->children, (intptr_t)then);

        return ifthenelse;
    }

    /* else */
    consume (parser);

    elsethen = parse_program_until_kw (parser, "end");

    if (!elsethen)
    {
        __destroy_dynarray_exprs(then->children);

        goto _CLEANUP_EXPRS;
    }

    ifthenelse = new_expr (EXPR_IF);

    if (!ifthenelse)
    {
        __destroy_dynarray_exprs(then->children);
        __destroy_dynarray_exprs(elsethen->children);

        goto _CLEANUP_EXPRS;
    }

    ifthenelse->children = new_dynarray_with_capacity (3);

    if (!ifthenelse->children.capacity)
    {
        __destroy_dynarray_exprs(then->children);
        __destroy_dynarray_exprs(elsethen->children);

        goto _CLEANUP_EXPRS;
    }

    push_dynarray (&ifthenelse->children, (intptr_t)cond);
    push_dynarray (&ifthenelse->children, (intptr_t)then);
    push_dynarray (&ifthenelse->children, (intptr_t)elsethen);

    return ifthenelse;

_CLEANUP_EXPRS:
    destroy_expr (cond);
    destroy_expr (then);
    destroy_expr (elsethen);
    destroy_expr (ifthenelse);

    return NULL;
}

static struct expr *
parse_while_do (parser)
	struct parser *parser;
{
    struct expr		*cond = NULL, *prg = NULL, *whiledo = NULL;
    struct lexeme	**kwtemp;

    /* while - assured to succeed */
    consume (parser);

    cond = parse_expr (parser);

    if (!cond)
        goto _CLEANUP_EXPRS;

    kwtemp = lookahead (parser, 0);

    if (!kwtemp)
        fprintf (stderr, "input: expected keyword `do', found end of file\n");
    else if (!IS_KEYWORD(kwtemp, "do"))
        __lexer_perror(*kwtemp, "expected keyword `do'\n");

    /* then */
    consume (parser);

    prg = parse_program_until_kw (parser, "end");

    if (!prg)
        goto _CLEANUP_EXPRS;

    whiledo = new_expr (EXPR_WHILE);

    if (!whiledo)
    {
        __destroy_dynarray_exprs(prg->children);
        goto _CLEANUP_EXPRS;
    }

    whiledo->children = new_dynarray_with_capacity (2);

    if (!whiledo->children.capacity)
    {
        __destroy_dynarray_exprs(prg->children);
        goto _CLEANUP_EXPRS;
    }

    push_dynarray (&whiledo->children, (intptr_t)cond);
    push_dynarray (&whiledo->children, (intptr_t)prg);

    return whiledo;

_CLEANUP_EXPRS:
    destroy_expr (cond);
    destroy_expr (prg);
    destroy_expr (whiledo);

    return NULL;
}

struct expr *
parse_one (parser)
	struct parser *parser;
{
    struct lexeme **next, **subs;

    next = lookahead (parser, 0);
    subs = lookahead (parser, 1);

    if (!next)
        return NULL;


    if (IS_KEYWORD(next, "if"))
        return parse_if_then (parser);


    if (IS_KEYWORD(next, "while"))
        return parse_while_do (parser);


    if ((*next)->type == IDENTIFIER
        && subs
        && (*subs)->type == BINDING)
        return parse_binding (parser);


    return parse_expr (parser);
}

struct expr *
new_expr (type)
	enum expr_type type;
{
    struct expr *expr = malloc (sizeof(struct expr));

    if (!expr)
        return NULL;

    expr->type = type;

    return expr;
}

void
destroy_expr (expr)
	struct expr *expr;
{
    size_t i;

    if (!expr)
        return;

    for (i = 0; i < expr->children.size; i++)
        destroy_expr ((struct expr *)expr->children.array[i]);

    free (expr);
}

#define PRINT_TABS(l) {\
                       size_t i;\
                       for (i = 0; i < l; i++)\
                           printf ("  ");\
}\

void
print_expr (expr, lvl)
	struct expr	*expr;
	size_t		lvl;
{
    size_t i;

    if (!expr)
        return;

    PRINT_TABS(lvl);

    switch (expr->type)
    {
    case EXPR_OP:
        printf ("EXPR_OP #%u\n", expr->value.op);
        break;
    case EXPR_BINDING:
        puts ("EXPR_BINDING");
        break;
    case EXPR_IDENTIFIER:
        printf ("EXPR_IDENTIFIER (%s)\n", expr->value.str);
        return;
    case EXPR_INTEGER:
        printf ("EXPR_INTEGER (%d)\n", expr->value.itg);
        return;
    case EXPR_FLOAT:
        printf ("EXPR_FLOAT (%ff)\n", expr->value.flt);
        return;
    case EXPR_PRIMCALL:
        printf ("EXPR_PRIMCALL #%lu\n", expr->value.prim);
        break;
    case EXPR_IF:
        puts ("IF-THEN-(ELSE)");
        break;
    case EXPR_WHILE:
        puts ("WHILE-DO");
        break;
    case EXPR_PROGRAM:
        puts ("BLOCK");
        break;
    }

    for (i = 0; i < expr->children.size; i++)
    {
        print_expr ((struct expr *)expr->children.array[i], lvl + 1);
    }
}

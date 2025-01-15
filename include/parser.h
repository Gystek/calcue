#ifndef PARSER_H_
# define PARSER_H_

# include <dynarray.h>
# include <lexer.h>
# include <stdint.h>

static size_t prim_arities[KW_N - PRIMS_START] = {
    1, /* print */
    0, /* dump */
    1, /* log */
    1, /* exp */
    1, /* sqrt */
};

/* statements are also referred to as expressions
 * it is not really that important
 */
enum expr_type {
    EXPR_OP,
    EXPR_BINDING,

    EXPR_IDENTIFIER,
    EXPR_INTEGER,
    EXPR_FLOAT,

    EXPR_PRIMCALL,

    EXPR_IF,
    EXPR_WHILE,
    EXPR_READ,

    EXPR_PROGRAM
};

struct expr {
    enum expr_type	type;
    union {
        char		*str;
        int32_t		itg;
        double		flt;
        enum operator	op;
        const char	*kw;
        size_t		prim;
    }			value;

    struct dynarray	children;
};

struct parser {
    struct lexeme	**input;
    size_t		input_n;
    size_t		cur;
};

struct parser	init_parser (struct lexeme **, size_t);

struct expr	*parse_program (struct parser *);

struct expr	*new_expr (enum expr_type);
void		destroy_expr (struct expr *);

void		print_expr (struct expr *, size_t);


#endif /* PARSER_H_ */

#ifndef LEXER_H_
# define LEXER_H_

# include <dynarray.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>

# define KW_N (9)

static const char *const keywords[KW_N] = {
    "read",
    "print",
    "dump",
    "if",
    "then",
    "else",

    "log",
    "exp",
    "sqrt"
};

enum lexeme_t {
    KEYWORD,
    IDENTIFIER,
    INTEGER,
    FLOAT,
    OPERATOR,
    L_PAR,
    R_PAR,
    BINDING,
    SEPARATOR,
};

enum operator {
    PLUS = '+',
    MINUS = '-',
    TIMES = '*',
    DIVISION = '/',
    POWER = '^',
    MODULO = '%',

    EQ = '=',
    NEQ, /* <> */
    GT, /* > */
    LT, /* < */
    GE, /* >= */
    LE, /* <= */

    AND = '&',
    OR  = '|',
};

struct lexeme {
   enum	lexeme_t	type;
   union {
       char		*str;
       int32_t		itg;
       double		flt;
       enum operator	op;
       size_t		kw_id;
   } 			value;
   uint16_t		line;
   uint16_t		column;
};

struct lexer {
    FILE	*f;

    char	last;

    uint16_t	line;
    uint16_t	column;

    struct dynarray lexemes;
};

struct lexer	init_lexer (FILE *);
struct dynarray	lex (struct lexer *);
int		destroy_lexer (struct lexer *);

void		print_lexeme (struct lexeme);
void		destroy_lexeme (struct lexeme);

# define __lexer_perror(lexer, ...) {\
 fprintf (stderr, "input %u:%u: ", lexer->line, lexer->column);\
 fprintf (stderr, __VA_ARGS__);\
}

#endif /* LEXER_H_ */

#ifndef LEXER_H_
# define LEXER_H_

# include <dynarray.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>

# define KW_N (12)
# define PRIMS_START (6)


/* Primitives are at the end of the keywords
 * array to facilitate access to them by the parser.
 * Increment PRIMS_START as you add non-primitive
 * (syntactical) keywords.
 */
static const char *const keywords[KW_N] = {
    "if",
    "then",
    "else",
    "end",

    "while",
    "do",

    /* primitives after this */

    "read",
    "print",
    "dump",

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
    NOT = '~'
};

struct lexeme {
   enum	lexeme_t	type;
   union {
       char		*str;
       int32_t		itg;
       double		flt;
       enum operator	op;
       const char	*kw;
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
 fprintf (stderr, "input %u:%u: ", (lexer)->line, (lexer)->column);\
 fprintf (stderr, __VA_ARGS__);\
}

#endif /* LEXER_H_ */

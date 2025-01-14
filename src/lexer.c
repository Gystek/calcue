#include <ctype.h>
#include <dynarray.h>
#include <errno.h>
#include <lexer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lexer
init_lexer (f)
	FILE *f;
{
    struct lexer lexer;

    lexer.f = f;
    lexer.line = 1;
    lexer.column = 1;
    lexer.lexemes = new_dynarray ();

    return lexer;
}

static char
lexer_peek (lexer)
	struct lexer *lexer;
{
    char c = fgetc (lexer->f);

    lexer->last = c;

    return c;
}

static void
lexer_push (lexer)
	struct lexer	*lexer;
{
    if (lexer->last == '\n')
    {
        lexer->line += 1;
        lexer->column = 0;
    }

    lexer->column += 1;
}

static char
lexer_get (lexer)
	struct lexer *lexer;
{
    char c = lexer_peek (lexer);

    lexer_push (lexer);

    return c;
}


#define MAX_NUMBER_LEX (512)

struct lexeme *
lex_number (lexer, lexeme, c)
	struct lexer	*lexer;
	struct lexeme	*lexeme;
	char		c;
{
    bool	isfloat = false;
    size_t	i = 0;
    char	buffer[MAX_NUMBER_LEX + 1] = {0};

    buffer[i++] = c;

    while (isdigit (lexer_peek (lexer)))
    {
        lexer_push (lexer);
        buffer[i++] = lexer->last;

        if (i > MAX_NUMBER_LEX)
        {
            __lexer_perror (lexer, "`%s' is too long of a number\n", buffer);

            return NULL;
        }
    }

    if (lexer->last == '.')
    {
        lexer_push (lexer);
        buffer[i++] = lexer->last;

        isfloat = true;

        if (i > MAX_NUMBER_LEX)
        {
            __lexer_perror (lexer, "`%s' is too long of a number\n", buffer);

            return NULL;
        }

        while (isdigit (lexer_peek (lexer)))
        {
            lexer_push (lexer);
            buffer[i++] = lexer->last;
    
            if (i > MAX_NUMBER_LEX)
            {
                __lexer_perror (lexer, "`%s' is too long of a number\n", buffer);
    
                return NULL;
            }
        }
    }

    ungetc (lexer->last, lexer->f);

    if (isfloat)
    {
        errno = 0;
        lexeme->type = FLOAT;
        lexeme->value.flt = strtod (buffer, NULL);

        if (errno)
        {
            __lexer_perror (lexer, "failed to convert `%s' to double float\n", buffer);

            return NULL;
        }

        return lexeme;
    }
    else
    {
        errno = 0;
        lexeme->type = INTEGER;
        lexeme->value.itg = strtol (buffer, NULL, 10);

        if (errno)
        {

            __lexer_perror (lexer, "failed to convert `%s' to 32-bits integer\n", buffer);

            return NULL;
        }

        return lexeme;
    }
}

#define MAX_IDENTIFIER_LEX (512)

static int
valid_identifier_c (c)
	char c;
{
    return (isalpha (c) || c == '_');
}

struct lexeme *
lex_alpha (lexer, lexeme, c)
	struct lexer	*lexer;
	struct lexeme	*lexeme;
	char		c;
{
    size_t	i = 0, j;
    char	buffer[MAX_IDENTIFIER_LEX + 1] = { 0 };

    buffer[i++] = c;

    while (valid_identifier_c (lexer_peek (lexer)))
    {
        lexer_push (lexer);
        buffer[i++] = lexer->last;

        if (i > MAX_IDENTIFIER_LEX)
        {
            __lexer_perror (lexer, "`%s' is too long of an identifier (max length: 512)\n", buffer);

            return NULL;
        }
    }

    ungetc (lexer->last, lexer->f);

    for (j = 0; j < KW_N; j++)
    {
        if (strcmp (buffer, keywords[j]) == 0)
        {
            lexeme->type = KEYWORD;
            lexeme->value.kw = keywords[j];

            return lexeme;
        }
    }

    lexeme->type = IDENTIFIER;
    lexeme->value.str = malloc (sizeof(char) * (i + 1));

    if (!lexeme->value.str)
        return NULL;

    strcpy (lexeme->value.str, buffer);

    return lexeme;
}

struct lexeme *
lex_next (lexer, lexeme)
	struct lexer	*lexer;
	struct lexeme	*lexeme;
{
    char		c;

    lexeme->line = lexer->line;
    lexeme->column = lexer->column;

    c = lexer_get (lexer);

    switch (c)
    {
    case '(':
    	lexeme->type = L_PAR;

    	return lexeme;
    case ')':
    	lexeme->type = R_PAR;

    	return lexeme;
    case EOF:
    case ';':
    case '\n':
        lexeme->type = SEPARATOR;

        return lexeme;
    case ' ':
    case '\t':
    case '\r':
    	return lex_next (lexer);
    case PLUS:
    case MINUS:
    case TIMES:
    case DIVISION:
    case POWER:
    case MODULO:
    case EQ:
    case OP_AND:
    case OP_NOT:
    case OR:
        lexeme->type = OPERATOR;
        lexeme->value.op = (enum operator)c;

        return lexeme;
    case ':':
    	lexeme->type = BINDING;

    	if (lexer_peek (lexer) == '=')
    	{
        	lexer_push (lexer);

        	return lexeme;
    	}

    	ungetc (lexer->last, lexer->f);

    	__lexer_perror (lexer, "unexpected character: `:'\n");

    	return NULL;
    case '<':
    	lexeme->type = OPERATOR;

    	if (lexer_peek (lexer) == '>')
    	{
        	lexer_push (lexer);

        	lexeme->value.op = NEQ;

        	return lexeme;
    	}

    	ungetc (lexer->last, lexer->f);

    	if (lexer_peek (lexer) == '=')
    	{
        	lexer_push (lexer);

        	lexeme->value.op = LE;

        	return lexeme;
    	}

    	ungetc (lexer->last, lexer->f);

    	lexeme->value.op = LT;

    	return lexeme;
    case '>':
    	lexeme->type = OPERATOR;

    	if (lexer_peek (lexer) == '=')
    	{
        	lexer_push (lexer);

        	lexeme->value.op = GE;

        	return lexeme;
    	}

    	ungetc (lexer->last, lexer->f);

    	lexeme->value.op = GT;

    	return lexeme;
    default:
        if (isdigit (c))
            return lex_number (lexer, lexeme, c);
        if (valid_identifier_c (c))
            return lex_alpha (lexer, lexeme, c);

        __lexer_perror (lexer, "unexpected character: `%c'\n", c);

        return NULL;
    }
}

struct dynarray
lex (lexer)
	struct lexer *lexer;
{
    lexer->last = 0;

    while (lexer->last != EOF)
    {
        struct lexeme *lexeme = malloc (sizeof(lexeme));

        if (!lexeme)
        {
            goto _FREE_CURRENT_LEXEMES;
        }

        if (!lex_next (lexer, lexeme))
        {
_FREE_CURRENT_LEXEMES:
            struct lexeme *to_free;

            while ((to_free = (struct lexeme *)pop_dynarray (&lexer->lexemes)))
            {
                free (to_free);
            }

            return new_dynarray ();
        }

        if (push_dynarray (&lexer->lexemes, (intptr_t)lexeme))
            goto _FREE_CURRENT_LEXEMES;
    }

    destroy_lexer (lexer);

    return lexer->lexemes;
}

int
destroy_lexer (lexer)
	struct lexer *lexer;
{
    if (lexer->f != stdin)
        fclose (lexer->f);

    return 0;
}

void
print_lexeme (lexeme)
	struct lexeme lexeme;
{
    printf ("%u:%u\t", lexeme.line, lexeme.column);

    switch (lexeme.type)
    {
    case L_PAR:
        puts ("LEFT PARENTHESIS");
        break;
    case R_PAR:
        puts ("RIGHT PARENTHESIS");
        break;
    case BINDING:
        puts ("BINDING SIGN");
        break;
    case SEPARATOR:
        puts ("SEPARATOR");
        break;
    case INTEGER:
        printf ("INT(%d)\n", lexeme.value.itg);
        break;
    case FLOAT:
        printf ("FLOAT(%ff)\n", lexeme.value.flt);
        break;
    case KEYWORD:
        printf ("KEYWORD(%s)\n", lexeme.value.kw);
        break;
    case IDENTIFIER:
        printf ("IDENTIFIER(%s)\n", lexeme.value.str);
        break;
    case OPERATOR:
        printf ("OPERATOR(");
        switch (lexeme.value.op)
        {
        case NEQ:
            printf ("<>");
            break;
        case GT:
            printf (">");
            break;
        case LT:
            printf ("<");
            break;
        case GE:
            printf (">=");
            break;
        case LE:
            printf ("<=");
            break;
        default:
            putchar ((char)lexeme.value.op);
            break;
        }
        puts (")");
    }
}

void
destroy_lexeme (lexeme)
	struct lexeme lexeme;
{
    if (lexeme.type == IDENTIFIER)
        free (lexeme.value.str);
}

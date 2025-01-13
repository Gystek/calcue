#include <errno.h>
#include <dynarray.h>
#include <lexer.h>
#include <stdlib.h>
#include <string.h>

int
main (argc, argv)
	int		argc;
	char *const	argv[];
{
    int i;

    for (i = 1; i < argc; i++)
    {
        FILE		*f;
        struct lexer	lexer;
        size_t		j;
        struct dynarray	lexemes;

        if (strcmp (argv[i], "-") == 0)
            f = stdin;
        else
        {
            f = fopen (argv[i], "r");

            if (!f)
            {
                perror ("calc");

                return EXIT_FAILURE;
            }
        }

        lexer = init_lexer (f);
        lexemes = lex (&lexer);

        for (j = 0; j < lexemes.size; j++)
        {
            struct lexeme *lexeme= (struct lexeme *)lexemes.array[j];

            print_lexeme (*lexeme);
            destroy_lexeme (*lexeme);
            free (lexeme);
        }

        destroy_dynarray (&lexemes);
    }

    return EXIT_SUCCESS;
}

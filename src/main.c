#include <errno.h>
#include <lex.h>
#include <list.h>
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
        struct list	lexemes;
        struct lexeme	*lexeme;

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

        while ((lexeme = (struct lexeme *)pop_list (&lexemes)))
        {
            print_lexeme (*lexeme);
            destroy_lexeme (*lexeme);
            free (lexeme);
        }
    }

    return EXIT_SUCCESS;
}

#include <compiler.h>
#include <dynarray.h>
#include <errno.h>
#include <lexer.h>
#include <parser.h>
#include <resolver.h>
#include <stdlib.h>
#include <string.h>

int
main (argc, argv)
	int		argc;
	char *const	argv[];
{
    int ret = 0;
    int i;

    for (i = 1; i < argc; i++)
    {
        FILE		*f;
        struct lexer	lexer;
        size_t		j;
        struct dynarray	lexemes;

        struct parser	parser;
        struct expr	*prg;

        struct bytecode	bc;

        struct dynarray	var_indices = new_dynarray ();

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

        #ifdef _DISPLAY_LEXING

        puts ("=== LEXING ===");

        #endif

        lexer = init_lexer (f);
        lexemes = lex (&lexer);

        #ifdef _DISPLAY_LEXING

        for (j = 0; j < lexemes.size; j++)
        {
            struct lexeme *lexeme= (struct lexeme *)lexemes.array[j];

            print_lexeme (*lexeme);
        }

        #endif

        #ifdef _DISPLAY_PARSING

        puts ("=== PARSING ===");

        #endif

        parser = init_parser ((struct lexeme **)lexemes.array, lexemes.size);

        prg = parse_program (&parser);

        #ifdef _DISPLAY_PARSING

        print_expr (prg, 0);

        #endif

        if ((ret = resolve (prg, &var_indices)))
            goto cleanup;

        bc = init_bytecode (&var_indices);

        if (compile (prg, &bc))
        {
            fprintf (stderr, "error: bytecode too long\n");
            goto cleanup;
        }

        bc.bytes[bc.byte_c++] = HLT;

        #ifdef _DISASSEMBLE

        disassembly (&bc);

        #endif

cleanup:

        destroy_dynarray (var_indices);

        destroy_expr (prg);

        for (j = 0; j < lexemes.size; j++)
        {
            struct lexeme *lexeme= (struct lexeme *)lexemes.array[j];

            destroy_lexeme (*lexeme);
            free (lexeme);
        }

        destroy_dynarray (lexemes);
    }

    return ret;
}

#include <compiler.h>
#include <dynarray.h>
#include <errno.h>
#include <lexer.h>
#include <parser.h>
#include <resolver.h>
#include <stdlib.h>
#include <string.h>
#include <vm.h>

int
run_file (f)
	FILE *f;
{
        struct lexer	lexer;
        size_t		j;
        struct dynarray	lexemes;

        struct parser	parser;
        struct expr	*prg;

        struct bytecode	bc;

        struct vm vm;

        struct dynarray	var_indices = new_dynarray ();

        lexer = init_lexer (f);
        lexemes = lex (&lexer);

        #ifdef _DISPLAY_LEXING

        puts ("=== LEXING ===");

        for (j = 0; j < lexemes.size; j++)
        {
            struct lexeme *lexeme= (struct lexeme *)lexemes.array[j];

            print_lexeme (*lexeme);
        }

        #endif

        parser = init_parser ((struct lexeme **)lexemes.array, lexemes.size);

        prg = parse_program (&parser);

        if (!prg)
            goto cleanup;

        #ifdef _DISPLAY_PARSING

        puts ("=== PARSING ===");

        print_expr (prg, 0);

        #endif

        if (resolve (prg, &var_indices))
            goto cleanup;


        bc = init_bytecode (&var_indices);

        if (compile (prg, &bc))
        {
            fprintf (stderr, "error: bytecode too long\n");
            goto cleanup;
        }

        bc.bytes[bc.byte_c++] = HLT;

        #ifdef _DISASSEMBLE

        puts ("=== DISASSEMBLY ===");

        disassembly (&bc);

        puts ("===================");

        #endif


        if (!init_vm (&bc, &vm))
            goto cleanup;

        if (run_vm (&vm) != 0)
        {
            destroy_vm (vm);
            goto cleanup;
        }

        #ifdef _DUMP_VM

        dump_vm (vm);

        #endif

        destroy_vm (vm);

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

        return 1;
}

#define VERSION ("0.1.0")

int
main (argc, argv)
	int		argc;
	char *const	argv[];
{
    int ret = 0;
    int i;

    if (argc == 1)
    {
        printf ("calcue %s\n", VERSION);
        puts ("Copyright (C) 2025 Gustek");
        puts ("This is free software with ABSOLUTELY NO WARRANTY.");
        return run_file (stdin);
    }

    for (i = 1; i < argc; i++)
    {
        FILE		*f;

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

        ret |= run_file (f);
     }

    return ret;
}

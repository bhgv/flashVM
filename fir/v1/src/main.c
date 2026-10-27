/* One explicit subcommand per operation keeps verification separate from
 * bytecode emission. Output is opened only after the input verifies. */
#include "fir.h"

#ifdef FIR_HAS_SYNTAX_FRONTEND
#include "firsyn.h"

#endif
#include <stdlib.h>
#include <string.h>


/* Validate CLI arguments, run the selected pipeline, and stage output on success. */
extern FILE *fir_temporary(void);

int main(int argc, char **argv)
{
    FILE *input, *output;
    char error[512] = "operation failed";
    int ok, check, optimise, compile, dis;
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("fir1 0.37.0 (FIR 1.0; SWO1; NC64 Dis loops, structured transfers and retained tables)");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("fir1 - FIR 1.0 compiler and verifier\n"
             "usage: fir1 check INPUT.fir\n"
#ifdef FIR_HAS_SYNTAX_FRONTEND
             "       fir1 syntax INPUT.fir (structural syntax only; no lowering)\n"
#endif
             "       fir1 print|opt|object|dis INPUT.fir OUTPUT\n"
             "check: syntax, SSA dominance, types and CFG; no output\n"
             "print: canonical FIR; opt: conservative FIR peephole\n"
             "object: SWO1 object; use swl to link SWF\n"
             "dis: direct NC64 Dis, scalar CFG, i32/bool/copy/comparison/wrap arithmetic, value+executed captures\n"
             "tables: direct cyclic big-table Dis with checked hosted Bigtable imports and GC maps.\n"
             "No implicit optimization. Use opt output explicitly.\n"
             "Required header: FIR 1.0; conditional: if %c, ^yes, ^no\n"
             "Comments: // and /* ... */. Channels require FlashVM extensions.\n"
             "Limits: one main() -> void, 4096 instructions/values, 256 blocks.\n"
             "TMPDIR selects secure temporary staging on Linux/Termux.\n"
             "Exit: 0 success, 1 pipeline/output failure, 2 CLI/setup failure.\n"
             "See fir/v1/doc/FORMAT.md and compiler/README.md (also .ru.md).");
        return 0;
    }
#ifdef FIR_HAS_SYNTAX_FRONTEND
    if (argc == 3 && !strcmp(argv[1], "syntax"))
    {
        FirSyntaxDiagnostic diagnostic;
        input = fopen(argv[2], "rb");
        if (!input)
        {
            perror(argv[2]);
            return 2;
        }
        ok = firsyn_read(input, &diagnostic);
        if (fclose(input))
        {
            return 2;
        }
        if (!ok)
        {
            fprintf(stderr,
                    "%s:%d:%d: %s E%d (byte %ld)\n",
                    argv[2],
                    diagnostic.line,
                    diagnostic.column,
                    diagnostic.message,
                    diagnostic.code,
                    diagnostic.offset);
        }
        return ok ? 0 : 1;
    }
#endif
    if (argc < 3)
    {
        goto usage;
    }
    check = !strcmp(argv[1], "check");
    optimise = !strcmp(argv[1], "opt");
    compile = !strcmp(argv[1], "object");
    dis = !strcmp(argv[1], "dis");
    if ((!check && !optimise && !compile && !dis && strcmp(argv[1], "print")) || argc != (check ? 3 : 4))
    {
        goto usage;
    }
    if (!check && !strcmp(argv[2], argv[3]))
    {
        fprintf(stderr, "input and output must be different paths\n");
        return 2;
    }
    input = fopen(argv[2], "rb");
    if (!input)
    {
        perror(argv[2]);
        return 2;
    }
    if (check)
    {
        ok = fir_run(FIR_CHECK, input, NULL, NULL, error, sizeof(error));
        fclose(input);
        if (!ok)
        {
            fprintf(stderr, "%s\n", error[0] ? error : "FIR output/I/O failure");
        }
        return ok ? 0 : 1;
    }
    /* Staging also protects against aliases/hard links to the input file. */
    output = fir_temporary();
    if (!output)
    {
        perror("temporary output");
        fclose(input);
        return 2;
    }
    ok = fir_run(compile    ? FIR_OBJECT
                 : dis      ? FIR_DIS
                 : optimise ? FIR_OPT
                            : FIR_PRINT,
                 input,
                 output,
                 NULL,
                 error,
                 sizeof(error));
    if (fclose(input))
    {
        ok = 0;
    }
    if (ok && (fflush(output) || fseek(output, 0, SEEK_SET)))
    {
        ok = 0;
    }
    if (ok)
    {
        FILE *destination = fopen(argv[3], "wb");
        if (!destination)
        {
            ok = 0;
        }
        else
        {
            unsigned char bytes[4096];
            size_t count;
            while ((count = fread(bytes, 1, sizeof(bytes), output)) != 0)
            {
                if (fwrite(bytes, 1, count, destination) != count)
                {
                    ok = 0;
                    break;
                }
            }
            if (ferror(output))
            {
                ok = 0;
            }
            if (fclose(destination))
            {
                ok = 0;
            }
        }
    }
    if (fclose(output))
    {
        ok = 0;
    }

    if (!ok)
    {
        fprintf(stderr, "%s\n", error[0] ? error : "FIR output/I/O failure");
    }
    return ok ? 0 : 1;
usage:
    fprintf(stderr, "usage: fir1 check INPUT\n       fir1 print|opt|object INPUT OUTPUT\n");
    return 2;
}

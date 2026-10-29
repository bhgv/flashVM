/* Exercise optimized target admission through the same public C99 libraries as IDE. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Inspect borrowed snapshots without retaining or changing IR. counter records
 * callback order; after must contain no checked identity operators. Return zero
 * on a broken contract, causing the pipeline to preserve its caller's output. */
static int snapshot(void *user, const char *phase, const FirModule *module)
{
    int *counter = user;
    int index;
    int checked = 0;
    if (strcmp(phase, *counter == 0 ? "before" : "after"))
    {
        return 0;
    }
    for (index = 0; index < module->count; ++index)
    {
        const FirInstruction *in = &module->instructions[index];
        if (!in->removed && (in->op == O_I32_DIV || in->op == O_I32_MOD || in->op == O_I32_SHL))
        {
            ++checked;
        }
    }
    if ((*counter == 0 && checked != 3) || (*counter == 1 && checked != 0))
    {
        return 0;
    }
    ++*counter;
    return 1;
}


/* Compile one source with FIR/debug sidecars. Raw SWO rejection must leave its
 * OLD prefix unchanged; optimized object/Dis must succeed via fir_run, with
 * before/after snapshots proving the transformation occurred inside the API.
 * All streams are locally owned and calls are serialized; return zero on error. */
static int exercise(void)
{
    const char *source = "implement M; M: module { value: fn(): int; }; value(): int {"
                         "sum:=0; for(i:=0;i<4;i++){sum+=(i/1)+(i%1)+(i<<0);} return sum+30;}";
    FILE *input = fir_temporary();
    FILE *fir = fir_temporary();
    FILE *debug = fir_temporary();
    FILE *output = fir_temporary();
    FirSourceDiagnostic diagnostic;
    FirRunOptions options = {1, snapshot, NULL};
    char error[256] = "";
    unsigned char magic[4];
    int task;
    int counter;
    int ok = 0;
    if (!input || !fir || !debug || !output || fputs(source, input) < 0 || fseek(input, 0, SEEK_SET) ||
        !fir_source_debug(input, fir, FIR_LIMBO, "api", error, sizeof(error), &diagnostic, debug) ||
        ftell(debug) <= 0 || fseek(fir, 0, SEEK_SET) || fputs("OLD", output) < 0 ||
        fir_run(FIR_OBJECT, fir, output, NULL, error, sizeof(error)) || !error[0] || ftell(output) != 3)
    {
        goto done;
    }
    for (task = FIR_OBJECT; task <= FIR_DIS; ++task)
    {
        FILE *artifact = fir_temporary();
        int accepted;
        counter = 0;
        options.user = &counter;
        if (!artifact)
        {
            goto done;
        }
        accepted = fseek(fir, 0, SEEK_SET) == 0 &&
                   fir_run((FirTask)task, fir, artifact, &options, error, sizeof(error)) && counter == 2 &&
                   ftell(artifact) > 4 && fseek(artifact, 0, SEEK_SET) == 0 && fread(magic, 1, 4, artifact) == 4;
        if (accepted && task == FIR_OBJECT)
        {
            accepted = memcmp(magic, "SWO1", 4) == 0;
        }
        fclose(artifact);
        if (!accepted)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "integer identity API failed: %s\n", error);
    }
    if (input)
    {
        fclose(input);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (debug)
    {
        fclose(debug);
    }
    if (output)
    {
        fclose(output);
    }
    return ok;
}


/* Repeat library compilation/optimization/rejection without parser state leaks. */
int main(void)
{
    int index;
    for (index = 0; index < 8; ++index)
    {
        if (!exercise())
        {
            return 1;
        }
    }
    puts("PASS integer identity C99 API: source/debug, target rejection, optimized SWO/Dis, snapshots, reset");
    return 0;
}

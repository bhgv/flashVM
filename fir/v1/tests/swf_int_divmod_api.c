/* Verify signed divmod admission and failure atomicity through public C99 FIR APIs. */
#include "fir.h"
#include <string.h>


/* Compile one borrowed divisor declaration into a caller-owned staged output.
 * operation/declaration/divisor identify valid FIR text, optimize selects the shared pass,
 * and expected says whether the backend must accept the module. Return zero
 * after a diagnostic on any mismatch. All streams are local and closed here;
 * the synchronous call starts no VM threads and retains no borrowed strings. */
static int exercise(const char *operation, const char *declaration, const char *divisor, int optimize, int expected)
{
    FILE *input = fir_temporary();           /* Locally owned source stream. */
    FILE *output = fir_temporary();          /* Locally owned artifact with a sentinel prefix. */
    FirRunOptions options = {0, NULL, NULL}; /* No callbacks or retained module state. */
    char error[256] = "";                    /* Bounded diagnostic owned by this invocation. */
    char bytes[8];                           /* Read back the sentinel and, on success, the object magic. */
    int accepted = 0;                        /* Actual public-pipeline acceptance. */
    int ok = 0;                              /* Return success only after validating stream position and bytes. */
    options.optimise = optimize;
    if (!input || !output ||
        fprintf(input,
                "FIR 1.0\nmodule @api\nfunc @main() -> void {\n^entry:\n"
                "%%x: i32 = const.i32 -7\n%s\n"
                "%%r: i32 = i32.%s.signed %%x, %s\ntrace %%r\nreturn\n}\n",
                declaration,
                operation,
                divisor) < 0 ||
        fseek(input, 0, SEEK_SET) || fputs("KEEP", output) < 0)
    {
        goto done;
    }
    accepted = fir_run(FIR_OBJECT, input, output, &options, error, sizeof(error));
    if (accepted != expected ||
        (!expected && (ftell(output) != 4 || !strstr(error, "proven nonzero literal divisor"))) ||
        fseek(output, 0, SEEK_SET) || fread(bytes, 1, expected ? 8 : 4, output) != (size_t)(expected ? 8 : 4) ||
        memcmp(bytes, "KEEP", 4) || (expected && memcmp(bytes + 4, "SWO1", 4)))
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr,
                "SWF signed divmod API mismatch: opt=%d expected=%d accepted=%d: %s\n",
                optimize,
                expected,
                accepted,
                error);
    }
    if (input)
    {
        fclose(input);
    }
    if (output)
    {
        fclose(output);
    }
    return ok;
}


/* Repeat mixed success/rejection calls to expose parser or preflight state leaks.
 * No arguments or external ownership are required. Return a failing exit status
 * on the first mismatch; no assertions or bootstrap compiler are involved. */
int main(void)
{
    int repeat;    /* Number of serialized public-library reuse rounds. */
    int operation; /* Exercise division and remainder through the same API. */
    for (repeat = 0; repeat < 8; ++repeat)
    {
        for (operation = 0; operation < 2; ++operation)
        {
            const char *name = operation ? "mod" : "div"; /* Borrowed static opcode suffix. */
            if (!exercise(name, "%n: i32 = const.i32 1", "%n", 0, 1) ||
                !exercise(name, "%n: i32 = const.i32 -1", "%n", 0, 1) ||
                !exercise(name, "%n: i32 = const.i32 -2147483648", "%n", 0, 1) ||
                !exercise(name, "%n: i32 = const.i32 0", "%n", 0, 0) ||
                !exercise(name, "%n: i32 = const.i32 0", "%n", 1, 0) ||
                !exercise(name, "%c: i32 = const.i32 2\n%n: i32 = copy %c", "%n", 0, 1) ||
                !exercise(name, "%c: i32 = const.i32 2\n%n: i32 = copy %c", "%n", 1, 1))
            {
                return 1;
            }
        }
    }
    puts("PASS SWF signed divmod C99 API: nonzero proof, zero rejection, optimizer, parser reuse");
    return 0;
}

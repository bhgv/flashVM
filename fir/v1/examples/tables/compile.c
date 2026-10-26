/* C99 embedding example: compile verified table FIR directly to NC64 Dis without a bootstrap. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

/* Compile argv[1] into argv[2] using borrowed public APIs and an owned staging stream.
 * Preflight failures do not open the destination. An output I/O failure can leave a partial file.
 * This host retires every stream/module. Returns 0 on success, 1 on compiler/I/O error, 2 on usage.
 * The resulting image requires the hosted Bigtable ABI, not the reference executor library. */
int main(int argc, char **argv)
{
    FirModule *module = NULL;
    FILE *input = NULL;
    FILE *staged = NULL;
    FILE *output = NULL;
    char error[512] = "table Dis compilation/I/O failed";
    char buffer[4096];
    size_t size;
    int ok = 0;
    if (argc != 3 || strcmp(argv[1], argv[2]) == 0)
    {
        fprintf(stderr, "usage: table-compile INPUT.fir OUTPUT.dis (different paths)\n");
        return 2;
    }
    module = malloc(sizeof(*module));
    input = fopen(argv[1], "rb");
    staged = fir_temporary();
    if (!module || !input || !staged || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_dis(staged, module, error, sizeof(error)) || fflush(staged) || fseek(staged, 0, SEEK_SET))
    {
        goto done;
    }
    output = fopen(argv[2], "wb");
    if (!output)
    {
        goto done;
    }
    while ((size = fread(buffer, 1, sizeof(buffer), staged)) != 0)
    {
        if (fwrite(buffer, 1, size, output) != size)
        {
            goto done;
        }
    }
    ok = !ferror(staged) && !ferror(output);
done:
    if (input)
    {
        fclose(input);
    }
    if (staged)
    {
        fclose(staged);
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    free(module);
    if (!ok)
    {
        fprintf(stderr, "%s\n", error);
    }
    return ok ? 0 : 1;
}

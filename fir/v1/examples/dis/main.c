/* Embed our source/FIR/Dis libraries in a native IDE build service; no subprocess compiler is used. */
#include "fir.h"
#include <string.h>


/* Translate borrowed Limbo input into caller-owned staged Dis output using our two C99 libraries.
 * All intermediate FIR storage is private and closed on every path. This call blocks for compilation;
 * an IDE should invoke it on its build worker and return copied results through its service channel.
 * Parsing calls require external serialization. Returns zero with a copied error on failure. */
static int compile(FILE *source, FILE *dis, char *error, size_t capacity)
{
    FILE *fir = fir_temporary();
    FirRunOptions options = {0, NULL, NULL}; /* No implicit optimization or callbacks in this example. */
    int ok;
    if (!fir)
    {
        snprintf(error, capacity, "cannot allocate intermediate FIR stream");
        return 0;
    }
    ok = fir_source(source, fir, FIR_LIMBO, "dis_example", error, capacity);
    if (ok && (fflush(fir) || fseek(fir, 0, SEEK_SET)))
    {
        ok = 0;
    }
    if (ok)
    {
        ok = fir_run(FIR_DIS, fir, dis, &options, error, capacity);
    }
    if (fclose(fir))
    {
        ok = 0;
    }
    return ok;
}


/* Compile argv[1] to argv[2] after staging the whole result; preserve old output on source rejection.
 * The program owns opened streams, closes all of them and returns 0/1/2 for success/failure/usage.
 * Final destination I/O is checked but cannot roll back an arbitrary filesystem write failure. */
int main(int argc, char **argv)
{
    FILE *source = NULL;
    FILE *staged = NULL;
    FILE *output = NULL;
    char error[256] = "compilation or I/O failed";
    unsigned char bytes[4096];
    size_t count;
    int ok = 0;
    if (argc != 3 || !strcmp(argv[1], argv[2]))
    {
        fprintf(stderr, "usage: fir1_dis_example SOURCE.b OUTPUT.dis (different paths)\n");
        return 2;
    }
    source = fopen(argv[1], "rb");
    staged = fir_temporary();
    if (!source || !staged || !compile(source, staged, error, sizeof(error)) || fflush(staged) ||
        fseek(staged, 0, SEEK_SET))
    {
        goto done;
    }
    output = fopen(argv[2], "wb");
    if (!output)
    {
        goto done;
    }
    ok = 1;
    while ((count = fread(bytes, 1, sizeof(bytes), staged)) != 0)
    {
        if (fwrite(bytes, 1, count, output) != count)
        {
            ok = 0;
            break;
        }
    }
    if (ferror(staged))
    {
        ok = 0;
    }
done:
    if (source && fclose(source))
    {
        ok = 0;
    }
    if (staged && fclose(staged))
    {
        ok = 0;
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    if (!ok)
    {
        fprintf(stderr, "%s\n", error);
    }
    return ok ? 0 : 1;
}

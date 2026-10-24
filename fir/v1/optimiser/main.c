/* Standalone optimizer CLI; the optimizer itself is the fir_run/FIR_OPT API. */
#include "fir.h"
#include <string.h>
extern FILE *fir_temporary(void);
int main(int argc, char **argv)
{
    FILE *in, *staged, *out;
    unsigned char data[4096];
    size_t n;
    int ok;
    char error[512] = "optimizer I/O failure";
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: fir1_opt INPUT.fir OUTPUT.fir\nC API: fir_run(FIR_OPT, ...), "
             "fir_optimise(...); no SWF output.");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("fir1_opt 0.5.2 (FIR 1.0)");
        return 0;
    }
    if (argc != 3 || !strcmp(argv[1], argv[2]))
        return 2;
    in = fopen(argv[1], "rb");
    if (!in)
        return 2;
    staged = fir_temporary();
    if (!staged)
    {
        fclose(in);
        return 2;
    }
    ok = fir_run(FIR_OPT, in, staged, NULL, error, sizeof(error));
    if (fclose(in))
        ok = 0;
    if (ok && (fflush(staged) || fseek(staged, 0, SEEK_SET)))
        ok = 0;
    if (ok)
    {
        out = fopen(argv[2], "wb");
        if (!out)
            ok = 0;
        else
        {
            while ((n = fread(data, 1, sizeof(data), staged)) != 0)
                if (fwrite(data, 1, n, out) != n)
                {
                    ok = 0;
                    break;
                }
            if (ferror(staged))
                ok = 0;
            if (fclose(out))
                ok = 0;
        }
    }
    fclose(staged);
    if (!ok)
        fprintf(stderr, "%s\n", error[0] ? error : "optimizer output/I/O failure");
    return ok ? 0 : 1;
}

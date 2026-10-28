/* CLI for the same library used by IDE/compiler clients; never a deployed dependency. */
#include <stdio.h>
#include "view.h"

/* Compile ASCII profile source into the shared FIR representation.
 * argv contains input/output paths. Return nonzero on I/O, syntax or emission failure.
 * Source is at most 16 KiB; Unicode document values arrive at execution through bindings. */
int main(int argc, char **argv)
{
    unsigned char source[16385];
    size_t size;
    size_t index;
    unsigned char fir_text[65536];
    size_t used;
    int failed;
    As2ViewDiagnostic diagnostic;
    FILE *input;
    FILE *output;
    if (argc != 3)
    {
        fprintf(stderr, "usage: as2_viewc source.as output.fir\n");
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (!input)
    {
        perror(argv[1]);
        return 1;
    }
    size = fread(source, 1, sizeof(source), input);
    failed = ferror(input) || size == sizeof(source);
    if (fclose(input))
    {
        failed = 1;
    }
    for (index = 0; index < size; index++)
    {
        if (source[index] == 0 || source[index] >= 128)
        {
            failed = 1;
        }
    }
    if (failed)
    {
        fprintf(stderr, "AS2 view source must be ASCII and at most 16384 bytes\n");
        return 1;
    }
    if (!as2_view_compile(source, size, fir_text, sizeof(fir_text), &used, &diagnostic))
    {
        fprintf(stderr,
                "AS2 view %d:%d: unsupported profile or error %d at %ld\n",
                diagnostic.line,
                diagnostic.column,
                diagnostic.code,
                diagnostic.offset);
        return 1;
    }
    output = fopen(argv[2], "wb");
    if (!output)
    {
        perror(argv[2]);
        return 1;
    }
    if (fwrite(fir_text, 1, used, output) != used)
    {
        fclose(output);
        remove(argv[2]);
        return 1;
    }
    failed = ferror(output);
    if (fclose(output))
    {
        failed = 1;
    }
    if (failed)
    {
        remove(argv[2]);
    }
    return failed ? 1 : 0;
}

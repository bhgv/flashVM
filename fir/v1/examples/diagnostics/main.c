/* A complete host-side diagnostic client. The generated compiler is a native
 * build tool, not part of the deployed Limbo IDE runtime. See README.md. */
#include "fir.h"
#include <string.h>


/* Compile an intentionally invalid source snapshot and inspect the first error.
 * This function owns both temporary streams and closes them on every path.
 * Caller-owned diagnostic structs contain scalars only and survive stream close. */
int main(void)
{
    /* The UTF-8 comment contains one three-byte character, followed by a tab.
     * 'missing' starts at byte 14, character 12: byte and character positions
     * cannot be interchanged by an editor. C escapes make byte encoding explicit. */
    const char text[] = "/*\xe4\xb8\xad*/\ttrace(missing);";
    FILE *input = fir_temporary();
    FILE *output = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char message[256];
    int ok = 0;
    if (!input || !output)
    {
        goto done;
    }
    if (fwrite(text, 1, strlen(text), input) != strlen(text) || fflush(input) || fseek(input, 0, SEEK_SET))
    {
        goto done;
    }
    /* A semantic rejection is expected. No FIR bytes should be published. */
    if (fir_source_ex(input, output, FIR_AS2, "diagnostic_example", message, sizeof(message), &diagnostic))
    {
        goto done;
    }
    if (diagnostic.kind != FIR_DIAG_SEMANTIC || !diagnostic.has_location || diagnostic.byte_offset != 14 ||
        diagnostic.byte_length != 7 || diagnostic.line != 1 || diagnostic.column != 15 || ftell(output) != 0)
    {
        goto done;
    }
    /* This output is for a person. Machine clients should consume the struct
     * directly, or opt into --diagnostics-json when running fir1_source. */
    printf("%s\nbyte span=[%zu,%zu), line=%zu, byte column=%zu\n",
           message,
           diagnostic.byte_offset,
           diagnostic.byte_offset + diagnostic.byte_length,
           diagnostic.line,
           diagnostic.column);
    puts("PASS source diagnostic example");
    ok = 1;
done:
    if (input)
    {
        fclose(input);
    }
    if (output)
    {
        fclose(output);
    }
    return ok ? 0 : 1;
}

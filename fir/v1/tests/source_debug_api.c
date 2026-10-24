/* Public stream ownership/rollback checks for the optional source debug API. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

/* Keep validation active in Release builds. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL source debug API: %s\n", message);
        exit(1);
    }
}

/* Exercise failed admission, complete debug publication and borrowed-root retirement. */
int main(void)
{
    int cycle;
    for (cycle = 0; cycle < 8; ++cycle)
    {
        FILE *input = fir_temporary(), *output = fir_temporary(), *debug = fir_temporary();
        FirSourceDiagnostic diagnostic;
        char message[128], buffer[4096];
        size_t size;
        require(input && output && debug, "streams");
        fputs("old FIR", output);
        fputs("old debug", debug);
        require(!fir_source_debug(input, output, FIR_AS2, NULL, message, sizeof(message),
                                   &diagnostic, output), "aliased output rejected");
        require(diagnostic.kind == FIR_DIAG_ARGUMENT && ftell(output) == 7, "argument rollback");
        require(!fir_source_debug(input, output, FIR_AS2, NULL, message, sizeof(message),
                                   &diagnostic, debug), "empty source rejected");
        require(ftell(output) == 7 && ftell(debug) == 9, "translation rollback of both streams");
        fclose(output);
        fclose(debug);
        output = fir_temporary();
        debug = fir_temporary();
        require(output && debug, "success streams");
        fputs("var x=1; x=-x; trace(x);", input);
        rewind(input);
        require(fir_source_debug(input, output, FIR_AS2, "debug_api", message, sizeof(message),
                                  &diagnostic, debug), "debug compilation");
        require(diagnostic.kind == FIR_DIAG_NONE, "success diagnostic");
        rewind(debug);
        size = fread(buffer, 1, sizeof(buffer) - 1, debug);
        buffer[size] = 0;
        require(strstr(buffer, "FIRDBG1") && strstr(buffer, "source-unoptimized-fir") &&
                strstr(buffer, "declare") && strstr(buffer, "assign"), "value/binding debug stream");
        fclose(debug);
        rewind(input);
        /* A plain call after debug is closed must not touch its retired FILE*. */
        require(fir_source(input, output, FIR_AS2, NULL, message, sizeof(message)), "plain call after debug close");
        fclose(input);
        fclose(output);
    }
    puts("PASS source debug API ownership: 8 cycles");
    return 0;
}

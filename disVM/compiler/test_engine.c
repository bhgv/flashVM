/* Exercise the public VM-independent service, ownership and recovery contract. */
#include "compiler_engine.h"
#include <stdio.h>
#include <string.h>


/* Reject a failed stage that leaked partial buffers or successful extents.
 * output borrows one completed result. Return one only for fully empty failure. */
static int failed_cleanly(const DvmCompileOutput *output)
{
    return output->stage != DVM_COMPILE_OK && !output->artifact && !output->debug && !output->artifact_size &&
           !output->debug_size && output->message[0];
}


/* Run bounded success/failure cycles or the explicitly invalid TMPDIR check.
 * argv is borrowed; no callbacks, VM or threads. Return nonzero with a concrete
 * stderr diagnostic. Every produced buffer retires, including failed checks. */
int main(int argc, char **argv)
{
    static const unsigned char source[] = "# π😀\nimplement M;M:module{value:fn():int;};value():int{"
                                          "sum:=0;for(i:=0;i<7;i++){sum+=6;}return sum;}";
    static const unsigned char bad[] = "implement M;M:module{value:fn():int;};value():int{return missing;}";
    static const unsigned char table[] = "implement M;M:module{value:fn():big;};value():big{return big 42;}";
    DvmCompileOutput output;
    int cycle;
    if (argc == 2 && !strcmp(argv[1], "--temp-failure"))
    {
        dvm_compile(4, 1, 0, source, sizeof(source) - 1, &output);
        if (output.stage != DVM_COMPILE_RESOURCE || !failed_cleanly(&output))
        {
            dvm_compile_dispose(&output);
            fputs("FAIL compiler service temporary staging admission\n", stderr);
            return 1;
        }
        dvm_compile_dispose(&output);
        return 0;
    }
    for (cycle = 0; cycle < 8; ++cycle)
    {
        int target;
        dvm_compile(4, 1, 0, bad, sizeof(bad) - 1, &output);
        if (output.stage != DVM_COMPILE_SOURCE || !failed_cleanly(&output) || output.line != 1 || output.length != 7 ||
            output.offset < 0 || !output.kind)
        {
            goto failed;
        }
        dvm_compile_dispose(&output);
        dvm_compile(4, 3, 0, table, sizeof(table) - 1, &output);
        if (output.stage != DVM_COMPILE_BACKEND || !failed_cleanly(&output))
        {
            goto failed;
        }
        dvm_compile_dispose(&output);
        dvm_compile(4, 1, 0, source, DVM_COMPILER_MAX_SOURCE + 1, &output);
        if (output.stage != DVM_COMPILE_LIMIT || !failed_cleanly(&output))
        {
            goto failed;
        }
        dvm_compile_dispose(&output);
        dvm_compile(4, 1, 2, source, sizeof(source) - 1, &output);
        if (output.stage != DVM_COMPILE_INVALID || !failed_cleanly(&output))
        {
            goto failed;
        }
        dvm_compile_dispose(&output);
        for (target = 1; target <= 3; ++target)
        {
            dvm_compile(4, target, cycle % 2, source, sizeof(source) - 1, &output);
            if (output.stage != DVM_COMPILE_OK || !output.artifact_size || !output.debug_size || !output.artifact ||
                !output.debug || output.message[0] || output.kind || output.offset != -1 ||
                (target == 2 && memcmp(output.artifact, "FIR", 3)) ||
                (target == 3 && memcmp(output.artifact, "FWS", 3)))
            {
                goto failed;
            }
            dvm_compile_dispose(&output);
            dvm_compile_dispose(&output);
        }
    }
    puts("PASS compiler service C99: 24 owned artifacts, 32 clean failures, recovery and repeated disposal");
    return 0;
failed:
    fprintf(stderr, "FAIL compiler service stage %d: %s\n", output.stage, output.message);
    dvm_compile_dispose(&output);
    return 1;
}

/* Optional compiler builtin: native libraries return copied artifacts to Dis. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "kernel.h"
#include "fircompilermod.h"
#include "sys_capture.h"
#include "../compiler/compiler_engine.h"
#include <stddef.h>


/* Exact generated eight-cell tuple; access through byte copies for packed slots. */
typedef struct
{
    WORD stage;      /* Result stage from Fircompiler constants. */
    Array *artifact; /* [GC:] Caller-mapped fresh byte array. */
    Array *debug;    /* [GC:] Caller-mapped fresh FIRDBG1 array. */
    String *message; /* [GC:] Caller-mapped copied diagnostic. */
    WORD kind;       /* Source diagnostic enum, zero otherwise. */
    WORD line;       /* One-based source line, zero if absent. */
    WORD offset;     /* Zero-based byte offset, minus one if absent. */
    WORD length;     /* Exact diagnostic span bytes. */
} CompilerResult;


/* Protected builder borrows a rooted result tuple and native-owned complete output. */
typedef struct
{
    void *result;             /* Borrowed caller result, never retained after native return. */
    DvmCompileOutput *output; /* Borrowed C buffers; outer callback always disposes them. */
} CompilerBuild;


/* [GC:] Clear all previous tuple roots before their destructors run. result is
 * borrowed, possibly packed caller storage. Reject malformed/multiply aliased
 * owners before mutation. No allocation or asynchronous work occurs. */
static void clear_result(void *result)
{
    CompilerResult empty;
    void *owners[3];
    owners[0] = dvm_sys_owner((uchar *)result + offsetof(CompilerResult, artifact));
    owners[1] = dvm_sys_owner((uchar *)result + offsetof(CompilerResult, debug));
    owners[2] = dvm_sys_owner((uchar *)result + offsetof(CompilerResult, message));
    dvm_sys_owners(owners, 3);
    memset(&empty, 0, sizeof(empty));
    empty.stage = DVM_COMPILE_RESOURCE;
    empty.offset = -1;
    empty.artifact = H;
    empty.debug = H;
    empty.message = H;
    memmove(result, &empty, sizeof(empty));
    destroy(owners[0]);
    destroy(owners[1]);
    destroy(owners[2]);
}


/* [GC:] Copy native bytes into a new mapped VM array, rooting it before any
 * later allocation. slot is borrowed packed return storage; bytes/size are
 * bounded native input. Allocation exceptions are handled by the outer protect. */
static void publish_bytes(void *slot, const unsigned char *bytes, size_t size)
{
    Array *array;
    if (!size)
    {
        return;
    }
    array = H2D(Array *, heaparray(&Tbyte, size));
    memmove(slot, &array, sizeof(array));
    memmove(array->data, bytes, size);
}


/* [GC:] Publish complete independently copied outputs under kprotect. argument
 * is a borrowed C builder. Return one after all allocations; success stage is
 * written last so partial construction can never advertise a successful build. */
static int publish_result(void *argument)
{
    CompilerBuild *build = argument;
    DvmCompileOutput *output = build->output;
    WORD value;
    publish_bytes((uchar *)build->result + offsetof(CompilerResult, artifact), output->artifact, output->artifact_size);
    publish_bytes((uchar *)build->result + offsetof(CompilerResult, debug), output->debug, output->debug_size);
    retstr(output->message, (uchar *)build->result + offsetof(CompilerResult, message));
    value = output->kind;
    memmove((uchar *)build->result + offsetof(CompilerResult, kind), &value, sizeof(value));
    value = output->line;
    memmove((uchar *)build->result + offsetof(CompilerResult, line), &value, sizeof(value));
    value = output->offset;
    memmove((uchar *)build->result + offsetof(CompilerResult, offset), &value, sizeof(value));
    value = output->length;
    memmove((uchar *)build->result + offsetof(CompilerResult, length), &value, sizeof(value));
    value = output->stage;
    memmove(build->result, &value, sizeof(value));
    return 1;
}


/* Compile a captured native-cell invocation on the sole VM owner. frame/inputs
 * stay rooted during this serialized blocking call. No scheduler unlock, process
 * launch or UI access occurs. Native buffers retire on every VM allocation error;
 * heap results are copied and rooted separately, never foreign pointers. */
void Fircompiler_compile(void *frame)
{
    F_Fircompiler_compile call;
    DvmCompileOutput output;
    CompilerBuild build;
    const unsigned char *source = NULL;
    size_t size = 0;
    int valid;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Fircompiler_compile, ret), sizeof(CompilerResult));
    dvm_sys_owner(&call.source);
    clear_result(call.ret);
    valid = call.language >= 1 && call.language <= 4 && call.target >= 1 && call.target <= 3 &&
            (call.optimize == 0 || call.optimize == 1);
    if (call.source != H)
    {
        if (D2H(call.source)->t != &Tarray || call.source->t == nil || call.source->t == H ||
            call.source->t->size != 1 || call.source->t->np != 0 || call.source->len < 0)
        {
            error(exType);
        }
        source = call.source->data;
        size = call.source->len;
    }
    dvm_compile(valid ? (int)call.language : 0,
                valid ? (int)call.target : 0,
                valid ? (int)call.optimize : 0,
                source,
                size,
                &output);
    build.result = call.ret;
    build.output = &output;
    if (!kprotect(publish_result, &build))
    {
        dvm_compile_dispose(&output);
        clear_result(call.ret);
        krethrow();
    }
    dvm_compile_dispose(&output);
}


/* Register the generated module only in explicitly linked compiler hosts.
 * No parameters/results, allocations beyond builtin registration or retained
 * parser contexts. Fail before exposing a mismatched native tuple contract. */
void dvm_compiler_initialize(void)
{
    F_Fircompiler_compile layout;
    if (sizeof(*layout.ret) != sizeof(CompilerResult) ||
        builtinmod("$Fircompiler", Fircompilermodtab, Fircompilermodlen) == nil)
    {
        error("compiler module registration/layout failed");
    }
}

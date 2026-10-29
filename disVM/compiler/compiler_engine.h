/* VM-independent C99 compiler service; only copied bytes cross into a VM. */
#ifndef DVM_COMPILER_ENGINE_H
#define DVM_COMPILER_ENGINE_H
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DVM_COMPILER_MAX_SOURCE 65536U   /* Exact input byte budget. */
#define DVM_COMPILER_MAX_RESULT 2097152U /* Independent artifact/debug output budgets. */


/* Returned stage; numeric values match the public Fircompiler module contract. */
typedef enum
{
    DVM_COMPILE_OK,       /* Complete independently owned outputs. */
    DVM_COMPILE_INVALID,  /* Invalid options or source representation. */
    DVM_COMPILE_SOURCE,   /* Located source syntax/semantic rejection. */
    DVM_COMPILE_VERIFY,   /* FIR parse, verification or optimization rejection. */
    DVM_COMPILE_BACKEND,  /* Selected object, Dis or linker rejection. */
    DVM_COMPILE_RESOURCE, /* Allocation, staging or output I/O failure. */
    DVM_COMPILE_LIMIT     /* Input/output size exceeds the service budget. */
} DvmCompileStage;


/* Caller-owned result. Initialize through dvm_compile; release with dispose.
 * Buffers contain exact byte counts, never borrowed parser/VM storage. */
typedef struct
{
    DvmCompileStage stage;   /* First failed stage or OK. */
    unsigned char *artifact; /* Owned full FIR/Dis/SWF, NULL after failure. */
    size_t artifact_size;    /* Exact artifact bytes. */
    unsigned char *debug;    /* Owned original FIRDBG1, NULL after failure. */
    size_t debug_size;       /* Exact debug bytes. */
    char message[512];       /* Copied plain diagnostic, always terminated. */
    int kind;                /* Public FirSourceDiagnosticKind, zero for non-source failures. */
    int line;                /* One-based source line or zero when absent. */
    long offset;             /* Zero-based UTF-8 source byte offset, -1 when absent. */
    size_t length;           /* Exact source span bytes, zero when absent. */
} DvmCompileOutput;


/* Compile borrowed bytes with language=1..4, target=1 Dis/2 FIR/3 SWF,
 * optimize=0/1. Replace uninitialized caller result with owned output; callers
 * must dispose any previous result first. Blocking/serialized: our source parser
 * is process-global. No VM, filesystem publication, subprocess or callback occurs. */
void dvm_compile(
    int language, int target, int optimize, const unsigned char *source, size_t size, DvmCompileOutput *output);


/* Release owned output buffers, zero extents and clear pointers. output may be
 * NULL; repeated disposal is safe. Does not own/close caller input bytes. */
void dvm_compile_dispose(DvmCompileOutput *output);
#ifdef __cplusplus
}
#endif
#endif

/* FlashVM: SWF/SWD source files and cold-path bytecode-to-source lookup. */
#ifndef FLASHVM_SOURCE_DEBUG_H
#define FLASHVM_SOURCE_DEBUG_H /* Public source debugger API guard. */
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Borrowed source view, valid until symbols are replaced/cleared or context destruction. */
typedef struct FVM_DebugSourcePosition
{
    unsigned long module_id; /* SWD script module identity. */
    const char *source;      /* Author-provided counted source name; never opened automatically. */
    size_t source_size;      /* Filename byte length. */
    unsigned long line;      /* One-based source line. */
    const char *line_text;   /* Borrowed line bytes without CR/LF. */
    size_t line_size;        /* Current line byte length. */
} FVM_DebugSourcePosition;


/* Decode FWS/CWS and attach one matching FWD6--8 SWD bundle atomically, outside execution.
     * DebugID must exist and match. New imports replace prior symbols; no file/network I/O.
     * Unknown records return UNSUPPORTED; malformed/ambiguous source data is never guessed. */
FVM_API FVM_Result fvm_debug_load_swd(FVM_Context *, const void *swf, size_t swf_size,
                                      const void *swd, size_t swd_size);


/* Remove the attached bundle outside execution. */
FVM_API FVM_Result fvm_debug_clear_symbols(FVM_Context *);


/* Number of source files embedded in the attached SWD. */
FVM_API FVM_Result fvm_debug_source_count(FVM_Context *, size_t *);


/* Borrow one source file and its original module ID/name/content. */
FVM_API FVM_Result fvm_debug_source_at(FVM_Context *, size_t, unsigned long *module_id,
                                       const char **name, size_t *name_size, const char **text,
                                       size_t *text_size);


/* Cold lookup accepts copied AVM1 bodies, so interpreter and JIT share the same mapping.
     * Identical bodies with conflicting locations return NOT_FOUND; work limits return UNSUPPORTED.
     */
FVM_API FVM_Result fvm_debug_source_position(FVM_Context *, const void *body, size_t body_size,
                                             unsigned long offset, FVM_DebugSourcePosition *);


/* Current execution position while inside a debugger/breakpoint callback or a native host call.
     */
FVM_API FVM_Result fvm_debug_current_source(FVM_Context *, FVM_DebugSourcePosition *);


/* Resolve a materialized register's SWD name at an action offset, without forcing a spill. */
FVM_API FVM_Result fvm_debug_register_name(FVM_Context *, const void *body, size_t body_size,
                                           unsigned long offset, unsigned int reg,
                                           const char **name, size_t *name_size);
#ifdef __cplusplus
}
#endif
#endif

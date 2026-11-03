/* Owner-local native cleanup boundaries; not managed roots or OS cancellation. */
#ifndef DVM_NATIVE_CLEANUP_H
#define DVM_NATIVE_CLEANUP_H
#include "disvm/native_context.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* Stable zeroed caller storage, private after link. It must outlive registration.
 * cleanup(argument) must unlink THIS record, return 1 and never throw/enter VM/
 * switch context. Registration during unwind refuses. Callback may free its own
 * storage after unlink: the dispatcher never reads the old record afterwards. */
typedef struct DVM_NativeCleanup
{
    DVM_NativeContext *context;       /* Exact owner, borrowed until unlink. */
    struct DVM_NativeCleanup *next;   /* Older registered record. */
    struct DVM_NativeCleanup *previous; /* Newer registered record. */
    unsigned long serial;            /* Monotonic context-local generation. */
    int (*cleanup)(void *argument);   /* Nonthrowing owner-local cleanup, no managed roots. */
    void *argument;                  /* Borrowed until unlink/callback completes. */
} DVM_NativeCleanup;


/* A checkpoint names a generation, not a pointer to a possibly closed scope.
 * Borrowed context must stay alive; never serialize or use after its destruction. */
typedef struct DVM_NativeCleanupMark
{
    DVM_NativeContext *context; /* Exact bound owner at capture. */
    unsigned long serial;      /* Last issued registration, even if later closed. */
} DVM_NativeCleanupMark;


/* Link private zeroed record with borrowed callback/argument on current context.
 * Return 1 on success, 0 unchanged for invalid/overflow/guarded/unwinding use.
 * No allocation, root publication or payload permission. */
int dvm_native_cleanup_link(DVM_NativeCleanup *record,
    int (*cleanup)(void *argument), void *argument);


/* Unlink current-owner record before retirement. Return 0 unchanged for foreign/
 * invalid/guarded state. Allowed during its unwind callback; no callback occurs. */
int dvm_native_cleanup_unlink(DVM_NativeCleanup *record);


/* Capture current generation into caller-owned mark. Return 0 unchanged without
 * a bound owner or during unwind/held guards. No allocation or managed access. */
int dvm_native_cleanup_mark(DVM_NativeCleanupMark *mark);


/* Return 1 iff mark is valid and no newer registration remains; otherwise 0.
 * Owner-local scalar query, no dangling record traversal or callback. */
int dvm_native_cleanup_clean(const DVM_NativeCleanupMark *mark);


/* Close newer records LIFO BEFORE native stack frames are abandoned. mark is
 * borrowed/current-owner; callback records must still be alive. Return 0 for
 * invalid role/mark/held guards/reentry. Callback failure can leave a partial
 * cleanup and is a fatal host protocol error, not transactional recovery. */
int dvm_native_cleanup_unwind(const DVM_NativeCleanupMark *mark);
#ifdef __cplusplus
}
#endif
#endif

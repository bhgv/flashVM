/* Actual HEADMP projection; resolver is retained process-live C validation. */
#ifndef DVM_SHARED_HEAD_H
#define DVM_SHARED_HEAD_H

struct Type; /* Actual immutable native metadata, defined by interp.h. */


/* Validate borrowed module and private WORD snapshot with original opcode error precedence.
 * Return borrowed validated Type, or raise. Caller roots the list and provides
 * current ACTIVE admission/module lifetime; no object guard spans invocation. */
typedef struct Type *(*DVM_HeadTypeResolver)(void *module, const void *index);


/* [GC:] Capture actual current REG operands once, hold/root source list under
 * its slot guard, validate Type/destination/payload, call mapped replacement,
 * and finish temporary ownership on success/error. Requires ACTIVE worker mode.
 * Return void normally; native/VM errors propagate after own metadata cleanup.
 * Resolver must be independently trusted C, not an arbitrary VM callback. */
void dvm_runtime_guarded_headmp(DVM_HeadTypeResolver resolver);
#endif

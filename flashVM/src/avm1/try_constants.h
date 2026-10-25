/* FlashVM: retained exception, binding and finally-return values for ActionTry. */
#ifndef FVM_AVM1_TRY_CONSTANTS_H
#define FVM_AVM1_TRY_CONSTANTS_H     /* Private ActionTry root-layout guard. */
#define FVM_AVM1_TRY_THROWN_ROOT 0U  /* [GC:] Caught value survives resetting the operand stack. */
#define FVM_AVM1_TRY_BINDING_ROOT 1U /* [GC:] Previous catch-name binding restored after catch. */
#define FVM_AVM1_TRY_RETURN_ROOT 1U  /* [GC:] After catch ends, the same slot retains a finally return. */
#define FVM_AVM1_TRY_PENDING_ROOT 2U /* [GC:] Pending exception retained while finally executes. */
#define FVM_AVM1_TRY_ROOT_COUNT 3U   /* [GC:] Number of saved values used in the two phases. */
#endif

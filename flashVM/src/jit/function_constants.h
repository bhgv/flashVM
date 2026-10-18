/* FlashVM: encoded function snapshots and transient publication roots. */
#ifndef FVM_JIT_FUNCTION_CONSTANTS_H
#define FVM_JIT_FUNCTION_CONSTANTS_H /* Private function-definition guard. */
#define FVM_JIT_FUNCTION_LENGTH_OFFSET 1U /* UI16 action payload length follows its opcode. */
#define FVM_JIT_FUNCTION_PAYLOAD_OFFSET 3U /* Opcode plus UI16 action length. */
#define FVM_JIT_FUNCTION_VALUE_ROOT 0U /* [GC:] Newly created closure through declaration callbacks. */
#define FVM_JIT_FUNCTION_NAME_ROOT 1U /* [GC:] Counted function binding name. */
#define FVM_JIT_FUNCTION_ROOT_COUNT 2U /* [GC:] Number of publication temporaries. */
#endif

/* FlashVM: immutable ActionTry descriptor header positions. */
#ifndef FVM_JIT_TRY_CONSTANTS_H
#define FVM_JIT_TRY_CONSTANTS_H /* Private native Try descriptor guard. */
#define FVM_JIT_TRY_LENGTH_OFFSET 1U /* UI16 header byte count follows the action opcode. */
#define FVM_JIT_TRY_PAYLOAD_OFFSET 3U /* First flag byte after opcode/UI16 length. */
#endif

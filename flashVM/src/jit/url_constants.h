/* FlashVM: adjacent immutable constants used by literal URL requests. */
#ifndef FVM_JIT_URL_CONSTANTS_H
#define FVM_JIT_URL_CONSTANTS_H               /* Native literal URL layout guard. */
#define FVM_JIT_URL_LITERAL_URL_OFFSET 0U     /* URL bytes relative to the first literal. */
#define FVM_JIT_URL_LITERAL_TARGET_OFFSET 1U  /* Target bytes following the URL literal. */
#define FVM_JIT_URL_LITERAL_CONSTANT_COUNT 2U /* Complete code-owned URL/target pair. */
#endif

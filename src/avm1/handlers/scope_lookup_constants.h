/* FlashVM: retained operands for lexical binding resolution. */
#ifndef FVM_SCOPE_LOOKUP_CONSTANTS_H
#define FVM_SCOPE_LOOKUP_CONSTANTS_H   /* Private lexical-lookup root-layout guard. */
#define FVM_SCOPE_LOOKUP_NAME_ROOT 0U  /* [GC:] Counted binding name through getter callbacks. */
#define FVM_SCOPE_LOOKUP_SCOPE_ROOT 1U /* [GC:] Original managed lexical chain. */
#define FVM_SCOPE_LOOKUP_ROOT_COUNT 2U /* [GC:] Number of lexical-lookup input roots. */
#endif

/* FlashVM: retained value and lexical parent during With scope construction. */
#ifndef FVM_WITH_SCOPE_CONSTANTS_H
#define FVM_WITH_SCOPE_CONSTANTS_H /* With service root-layout guard. */
#define FVM_WITH_SCOPE_VALUE_ROOT                                                                  \
    0U /* [GC:] Input primitive replaced by its wrapper or bare object. */
#define FVM_WITH_SCOPE_PARENT_ROOT                                                                 \
    1U                               /* [GC:] Enclosing lexical scope through boxing callbacks.    \
                                      */
#define FVM_WITH_SCOPE_ROOT_COUNT 2U /* [GC:] Number of roots before node publication. */
#endif

/* FlashVM: shared counted variable-reference syntax and bounded token count. */
#ifndef FVM_REFERENCE_PATH_CONSTANTS_H
#define FVM_REFERENCE_PATH_CONSTANTS_H /* Private reference-syntax guard. */
#define FVM_REFERENCE_PATH_DOT '.'     /* SWF5+ variable separator; Delete2 also splits on it in SWF4. */
#define FVM_REFERENCE_PATH_COLON ':'   /* Legacy variable/member separator. */
#define FVM_REFERENCE_PATH_SLASH '/'   /* Object/display-path separator; final member slashes are literal. */
#define FVM_REFERENCE_PATH_MAX_SEGMENTS                                                                                \
    1024U                              /* Reject excessive qualified names before getters.  \
                                               */
#endif

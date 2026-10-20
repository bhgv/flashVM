/* Private deferred owner-thread operation identities; never serialized as AVM1 flags. */
#ifndef FVM_DESKTOP_REQUEST_CONSTANTS_H
#define FVM_DESKTOP_REQUEST_CONSTANTS_H
#define FVM_DESKTOP_REQUEST_UNLOAD 65536U     /* Separate from supported AVM1 GetURL2 flags. */
#define FVM_DESKTOP_UNLOAD_MAX_ARGS 1U        /* Zero-argument receiver or one explicit target. */
#define FVM_DESKTOP_NUMBERED_LOAD_MIN_ARGS 2U /* URL plus numeric level. */
#define FVM_DESKTOP_NUMBERED_LOAD_MAX_ARGS 3U /* Optional explicit GET selector. */
#define FVM_DESKTOP_NUMBERED_LEVEL_INDEX 1U   /* Level operand follows the URL. */
#define FVM_DESKTOP_NUMBERED_METHOD_INDEX 2U  /* Optional method follows the level. */
#endif

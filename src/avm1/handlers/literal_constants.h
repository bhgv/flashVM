/* FlashVM: value-only literal construction root layouts, separate from Action operands. */
#ifndef FVM_EXEC_LITERAL_CONSTANTS_H
#define FVM_EXEC_LITERAL_CONSTANTS_H       /* Literal service root layout guard. */
#define FVM_LITERAL_RESULT_ROOT 0U         /* [GC:] Partially constructed Array/Object. */
#define FVM_LITERAL_ARRAY_VALUE_BASE 1U    /* [GC:] Copied Array elements follow the result. */
#define FVM_LITERAL_OBJECT_KEY_ROOT 1U     /* [GC:] Current converted counted property name. */
#define FVM_LITERAL_OBJECT_ENTRY_BASE 2U   /* [GC:] Copied name/value pairs follow fixed roots. */
#define FVM_LITERAL_OBJECT_ENTRY_WIDTH 2U  /* [GC:] Each retained pair contains two values. */
#define FVM_LITERAL_OBJECT_VALUE_OFFSET 1U /* [GC:] Value follows the name in a copied pair. */
#endif

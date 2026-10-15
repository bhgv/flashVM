/* FlashVM: AVM1 literal entry layouts used only by the bytecode adapter. */
#ifndef FVM_AVM1_LITERAL_CONSTANTS_H
#define FVM_AVM1_LITERAL_CONSTANTS_H           /* Literal-layout header guard. */
#define FVM_AVM1_LITERAL_COUNT_OPERANDS 1U     /* Count above the Array elements or Object pairs. */
#define FVM_AVM1_ARRAY_LITERAL_ENTRY_WIDTH 1U  /* One value for each Array entry. */
#define FVM_AVM1_ARRAY_LITERAL_FIRST_VALUE 2U  /* First element lies immediately below count. */
#define FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH 2U /* Each Object entry supplies name and value. */
#define FVM_AVM1_OBJECT_LITERAL_FIRST_VALUE 2U /* First pair value lies below count. */
#define FVM_AVM1_OBJECT_LITERAL_FIRST_NAME 3U  /* First pair name lies below its value. */
#define FVM_AVM1_OBJECT_LITERAL_NAME_IN_PAIR 1U /* Name follows value when popping a pair. */
#endif

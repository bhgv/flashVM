/* FlashVM: rooted operand layout for delete, logical and legacy comparison services. */
#ifndef FVM_VALUE_OPERATION_CONSTANTS_H
#define FVM_VALUE_OPERATION_CONSTANTS_H     /* Private layout guard. */
#define FVM_EXEC_VALUE_LEFT_ROOT 0U         /* First operand or property owner. */
#define FVM_EXEC_VALUE_RIGHT_ROOT 1U        /* Second operand or coercible property name. */
#define FVM_EXEC_VALUE_TEMP_ROOT 2U         /* Converted property name retained through deletion. */
#define FVM_EXEC_VALUE_BINARY_ROOT_COUNT 2U /* Root count for binary and unary value services. */
#define FVM_EXEC_VALUE_DELETE_ROOT_COUNT 3U /* Root count including converted deletion name. */
/* Root count for a coercible call count. */
#define FVM_EXEC_VALUE_UNARY_ROOT_COUNT 1U
#endif

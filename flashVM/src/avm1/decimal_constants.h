/* FlashVM: decimal digit scaling and binary exponent traversal, independent of radix inference. */
#ifndef FVM_AVM1_DECIMAL_CONSTANTS_H
#define FVM_AVM1_DECIMAL_CONSTANTS_H       /* Guard for decimal representation constants. */
#define FVM_DECIMAL_RADIX 10U              /* Decimal place-value base. */
#define FVM_DECIMAL_POWER_SELECTED_BIT 1UL /* Lowest exponent bit selects scaling. */
#define FVM_DECIMAL_POWER_STEP_BITS 1U     /* Each squaring consumes one exponent bit. */
#endif

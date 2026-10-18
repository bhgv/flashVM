/* FlashVM: explicit interface declaration roots and bounds, independent of operand layout. */
#ifndef FVM_IMPLEMENTS_CONSTANTS_H
#define FVM_IMPLEMENTS_CONSTANTS_H /* Interface declaration constants guard. */
#define FVM_IMPLEMENTS_CLASS_ROOT                                                                  \
    0U /* [GC:] Class value retained before reading own prototype.                                 \
        */
#define FVM_IMPLEMENTS_ARGUMENT_BASE                                                               \
    1U                                 /* [GC:] Original interface values follow the class root.   \
                                        */
#define FVM_IMPLEMENTS_COUNT_NUMBER 0U /* Host count conversion retains Number validation. */
#define FVM_IMPLEMENTS_COUNT_INT32 1U  /* Loaded counts wrap to signed Int32 before clamping. */
#endif

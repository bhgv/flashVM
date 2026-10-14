/* FlashVM: identity-check roots, raw prototype name and bounded graph traversal. */
#ifndef FVM_INSTANCE_OF_CONSTANTS_H
#define FVM_INSTANCE_OF_CONSTANTS_H         /* InstanceOf service constants guard. */
#define FVM_INSTANCE_OF_VALUE_ROOT 0U       /* [GC:] Original tested value through class boxing. */
#define FVM_INSTANCE_OF_CLASS_ROOT 1U       /* [GC:] Original class operand, before coercion. */
#define FVM_INSTANCE_OF_BOXED_CLASS_ROOT 2U /* [GC:] Class object after primitive boxing. */
#define FVM_INSTANCE_OF_PROTOTYPE_ROOT 3U   /* [GC:] Raw prototype, then its boxed representation. \
                                             */
#define FVM_INSTANCE_OF_ROOT_COUNT 4U       /* [GC:] Fixed roots before graph traversal. */
#define FVM_INSTANCE_OF_PROTOTYPE_NAME "prototype" /* Own stored class data, without getters. */
/* UTF-16 unit count of the ASCII raw prototype field name. */
#define FVM_INSTANCE_OF_PROTOTYPE_UNITS (sizeof(FVM_INSTANCE_OF_PROTOTYPE_NAME) - 1U)
#define FVM_INSTANCE_OF_MAX_INTERFACE_NODES 4096U /* Bound visited interface nodes/cycles. */
#endif

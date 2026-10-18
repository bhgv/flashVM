/* FlashVM: identity-check roots, raw prototype name and graph traversal. */
#ifndef FVM_INSTANCE_OF_CONSTANTS_H
#define FVM_INSTANCE_OF_CONSTANTS_H         /* InstanceOf service constants guard. */
#define FVM_INSTANCE_OF_VALUE_ROOT 0U       /* [GC:] Original tested value through class boxing. */
#define FVM_INSTANCE_OF_CLASS_ROOT 1U       /* [GC:] Original class operand, before coercion. */
#define FVM_INSTANCE_OF_BOXED_CLASS_ROOT 2U /* [GC:] Class object after primitive boxing. */
#define FVM_INSTANCE_OF_PROTOTYPE_ROOT                                                             \
    3U                                /* [GC:] Raw prototype, then its boxed representation.       \
                                       */
#define FVM_INSTANCE_OF_ROOT_COUNT 4U /* [GC:] Fixed roots before graph traversal. */
#endif

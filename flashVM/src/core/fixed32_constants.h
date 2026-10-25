/* FlashVM: modular word masks for portable signed 16.16 scalar products. */
#ifndef FVM_FIXED32_CONSTANTS_H
#define FVM_FIXED32_CONSTANTS_H            /* Private modular arithmetic guard. */
#define FVM_FIXED32_WORD_MASK 0xffffffffUL /* Retain the low 32 result bits on wider hosts. */
#define FVM_FIXED32_LIMB_MASK 0xffffUL     /* Each unsigned multiplication limb contains 16 bits. */
#define FVM_FIXED32_LIMB_SIGN 0x8000UL     /* Signed high-limb sign bit before extension. */
#define FVM_FIXED32_EXTEND_MASK                                                                                        \
    0xffff0000UL                           /* Extend signed high limbs within the low 32 bits.   \
                                              */
#endif

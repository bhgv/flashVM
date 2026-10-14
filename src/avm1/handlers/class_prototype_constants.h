/* FlashVM: own stored class-prototype field identity, shared by class operations. */
#ifndef FVM_CLASS_PROTOTYPE_CONSTANTS_H
#define FVM_CLASS_PROTOTYPE_CONSTANTS_H      /* Class prototype field guard. */
#define FVM_CLASS_PROTOTYPE_NAME "prototype" /* Data field, never an inherited/accessor read. */
#define FVM_CLASS_PROTOTYPE_UNITS (sizeof(FVM_CLASS_PROTOTYPE_NAME) - 1U) /* ASCII UTF-16 units.   \
                                                                           */
#endif

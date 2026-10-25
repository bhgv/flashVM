/* FlashVM: AVM1 stored attribute width and SWF property visibility masks. */
#ifndef FVM_CORE_PROPERTY_VERSION_CONSTANTS_H
#define FVM_CORE_PROPERTY_VERSION_CONSTANTS_H  /* Private property version guard. */
#define FVM_PROPERTY_AVM1_FLAGS_MASK 0xffffU   /* ASSetPropFlags retains all sixteen bits. */
#define FVM_PROPERTY_AVM1_VERSION_MASK 0xfff8U /* Writable data replacement clears version requirements. */
#define FVM_PROPERTY_HIDE_SWF4 0x7ff8U         /* SWF4 and earlier visibility exclusion bits. */
#define FVM_PROPERTY_HIDE_SWF5 0x7480U         /* SWF5 visibility exclusion bits. */
#define FVM_PROPERTY_HIDE_SWF6 0x7500U         /* SWF6 visibility exclusion bits. */
#define FVM_PROPERTY_HIDE_SWF7 0x7000U         /* SWF7 visibility exclusion bits. */
#define FVM_PROPERTY_HIDE_SWF8 0x6000U         /* SWF8 visibility exclusion bits. */
#endif

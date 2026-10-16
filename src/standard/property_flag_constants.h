/* FlashVM: numeric narrowing bounds for loaded ASSetPropFlags arguments. */
#ifndef FVM_STANDARD_PROPERTY_FLAG_CONSTANTS_H
#define FVM_STANDARD_PROPERTY_FLAG_CONSTANTS_H /* Private flag conversion guard. */
#define FVM_AS_PROPERTY_FLAGS_TARGET_ARGUMENT 0U /* Object operand before any other conversion. */
#define FVM_AS_PROPERTY_FLAGS_NAMES_ARGUMENT 1U /* Null/all or comma-separated property list. */
#define FVM_AS_PROPERTY_FLAGS_SET_ARGUMENT 2U /* Flags applied after clearing. */
#define FVM_AS_PROPERTY_FLAGS_CLEAR_ARGUMENT 3U /* Flags removed before setting. */
#define FVM_AS_PROPERTY_FLAGS_INDEX_KEY_BYTES 32U /* Existing decimal unsigned-long key buffer bound. */
#define FVM_AS_PROPERTY_FLAGS_UINT16_MAX_NUMBER 65535.0 /* Saturating unsigned 16-bit conversion limit. */
#endif

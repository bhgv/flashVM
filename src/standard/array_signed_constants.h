/* FlashVM: signed property-key bounds shared by loaded borrowed Array methods. */
#ifndef FVM_ARRAY_SIGNED_CONSTANTS_H
#define FVM_ARRAY_SIGNED_CONSTANTS_H /* Header guard. */
/* Decimal storage covers the host long too, so formatting needs no range inference. */
#define FVM_ARRAY_SIGNED_KEY_CAPACITY (sizeof(long) * 3U + 2U)
/* Decimal radix for wrapping signed property-name parsing. */
#define FVM_ARRAY_INDEX_RADIX 10UL
#endif

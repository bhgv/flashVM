/* FlashVM: temporary root layouts and indexing for legacy string operations. */
#ifndef FVM_STRING_OPERATION_CONSTANTS_H
#define FVM_STRING_OPERATION_CONSTANTS_H /* Shared string-operation layout guard. */
#define FVM_STRING_UNARY_INPUT_ROOT 0U   /* [GC:] Operand retained during string/number coercion. */
#define FVM_STRING_UNARY_ROOT_COUNT 1U   /* [GC:] One input root for a unary string operation. */
#define FVM_STRING_EXTRACT_INPUT_ROOT 0U /* [GC:] Input retained while length/start convert. */
#define FVM_STRING_EXTRACT_START_ROOT 1U /* [GC:] One-based starting position operand. */
#define FVM_STRING_EXTRACT_LENGTH_ROOT 2U /* [GC:] Requested length, converted first. */
#define FVM_STRING_EXTRACT_ROOT_COUNT 3U  /* [GC:] All extraction operands are retained together.  \
                                           */
#define FVM_STRING_EXTRACT_FIRST_POSITION 1U /* Legacy substring positions begin at one. */
#endif

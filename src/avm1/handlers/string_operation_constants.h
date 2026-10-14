/* FlashVM: temporary root layouts and indexing for legacy string operations. */
#ifndef FVM_STRING_OPERATION_CONSTANTS_H
#define FVM_STRING_OPERATION_CONSTANTS_H /* Shared string-operation layout guard. */
#define FVM_STRING_UNARY_INPUT_ROOT 0U   /* [GC:] Operand retained during string/number coercion. */
#define FVM_STRING_UNARY_ROOT_COUNT 1U   /* [GC:] One input root for a unary string operation. */
#define FVM_STRING_EXTRACT_INPUT_ROOT 0U /* [GC:] Input retained while length/start convert. */
#define FVM_STRING_EXTRACT_START_ROOT 1U /* [GC:] One-based starting position operand. */
#define FVM_STRING_EXTRACT_LENGTH_ROOT 2U /* [GC:] Requested length, converted first. */
#define FVM_STRING_EXTRACT_ROOT_COUNT                                                              \
    3U /* [GC:] All extraction operands are retained together.                                     \
        */
#define FVM_STRING_EXTRACT_FIRST_POSITION 1U /* Legacy substring positions begin at one. */
/* Binary string operations retain original operands and both converted results separately. */
#define FVM_STRING_BINARY_LEFT_ROOT 0U  /* [GC:] Left input retained during right conversion. */
#define FVM_STRING_BINARY_RIGHT_ROOT 1U /* [GC:] Right input retained during left conversion. */
#define FVM_STRING_BINARY_LEFT_TEXT_ROOT                                                           \
    2U /* [GC:] Converted left text survives the other callback. */
#define FVM_STRING_BINARY_RIGHT_TEXT_ROOT                                                          \
    3U /* [GC:] Converted right text survives the other callback. */
#define FVM_STRING_BINARY_ROOT_COUNT 4U /* [GC:] Both inputs and converted strings. */
#endif

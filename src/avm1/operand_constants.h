/* FlashVM: encoded byte widths, signed branches and fixed AVM1 stack operand counts. */
#ifndef FVM_AVM1_OPERAND_CONSTANTS_H
#define FVM_AVM1_OPERAND_CONSTANTS_H /* Operand-layout guard. */
#define FVM_AVM1_BYTE_BITS 8U    /* Bits in each encoded byte, independent of host word width. */
#define FVM_AVM1_WORD_BYTES 2U   /* UI16 operand byte length. */
#define FVM_AVM1_DWORD_BYTES 4U  /* UI32 operand and each half of an encoded double. */
#define FVM_AVM1_STRING_BINARY_OPERANDS 2U /* StringAdd/comparisons consume left and right values. */
#define FVM_AVM1_STRING_EXTRACT_OPERANDS 3U /* StringExtract consumes input, start and length. */
#define FVM_AVM1_STRING_EXTRACT_START_OFFSET 2U /* Start lies below the top length operand. */
#define FVM_AVM1_DOUBLE_BYTES 8U /* Two little-endian words, high word first. */
/* Named construction has a separate layout identity from ordinary invocation. */
#define FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS 2U /* NewObject has count and name above arguments. */
#define FVM_AVM1_NEW_OBJECT_FIRST_ARGUMENT 3U /* Offset below constructor name/count to arg zero. */
#define FVM_AVM1_NAMED_CALL_FIXED_OPERANDS 2U /* CallFunction has count and name above arguments. */
#define FVM_AVM1_NAMED_CALL_FIRST_ARGUMENT 3U /* Offset below name/count to the first call argument. */
#define FVM_AVM1_DECLARE_INITIALIZED_OPERANDS 2U /* DefineLocal consumes name and initializer. */
#define FVM_AVM1_DECLARE_ONLY_OPERANDS 1U /* DefineLocal2 consumes only its name. */
#define FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT 2U /* SetVariable consumes name followed by value. */
#define FVM_AVM1_BRANCH_BYTES 2U /* Signed UI16 relative jump payload length. */
#define FVM_AVM1_BRANCH_SIGN_BIT 0x8000L /* Upper half of the signed branch encoding. */
#define FVM_AVM1_BRANCH_MODULUS 65536L    /* Subtract 2^16 to sign-extend a branch into long. */
#define FVM_AVM1_CAST_OPERANDS 2U /* CastOp consumes class followed by the original value. */
#define FVM_AVM1_INSTANCE_OF_OPERANDS 2U /* InstanceOf consumes value followed by class. */
#define FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS 2U /* Interface count and class above interface values. */
#define FVM_AVM1_IMPLEMENTS_FIRST_INTERFACE 3U /* Offset below class to the first interface value. */
#endif

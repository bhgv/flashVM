/* FlashVM: byte widths and signed branch representation in AVM1 operands. */
#ifndef FVM_AVM1_OPERAND_CONSTANTS_H
#define FVM_AVM1_OPERAND_CONSTANTS_H /* Operand-layout guard. */
#define FVM_AVM1_BYTE_BITS 8U    /* Bits in each encoded byte, independent of host word width. */
#define FVM_AVM1_WORD_BYTES 2U   /* UI16 operand byte length. */
#define FVM_AVM1_DWORD_BYTES 4U  /* UI32 operand and each half of an encoded double. */
#define FVM_AVM1_DOUBLE_BYTES 8U /* Two little-endian words, high word first. */
#define FVM_AVM1_BRANCH_BYTES 2U /* Signed UI16 relative jump payload length. */
#define FVM_AVM1_BRANCH_SIGN_BIT 0x8000L /* Upper half of the signed branch encoding. */
#define FVM_AVM1_BRANCH_MODULUS 65536L    /* Subtract 2^16 to sign-extend a branch into long. */
#endif

/* FlashVM: portable optional resource-error metadata codec for broker replies. */
#ifndef FVM_CORE_FAULT_WIRE_HPP
#define FVM_CORE_FAULT_WIRE_HPP /* Guard internal authority fault encoding declarations. */
#include "flashvm/exception.h"


/* FFME v1 stores explicit Uint32 fields followed by fixed terminated diagnostic text arrays. */
enum
{
    FVM_FAULT_WIRE_SIZE = 1220 /* Bytes in one complete optional authority diagnostic record. */
};


/* Encode existing bounded diagnostic fields without allocation; data must hold the full record. */
void fvm_fault_wire_encode(unsigned char *, const FVM_ExceptionInfo &);


/* Validate the complete record before updating fault metadata; preserve caller location and status.
 */
bool fvm_fault_wire_decode(FVM_ExceptionInfo *, const unsigned char *, size_t);
#endif

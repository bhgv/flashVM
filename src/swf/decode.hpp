#ifndef FLASHVM_SWF_DECODE_HPP
#define FLASHVM_SWF_DECODE_HPP
#include "flashvm/swf.h"
#include "../details/vector.hpp"
/* Decode into canonical FWS bytes. The limit is applied before allocation. */
FVM_Result fvm_swf_decode(const void *, size_t, size_t, fvm_detail::vector<unsigned char> &,
                          FVM_SWFInfo *);
#endif

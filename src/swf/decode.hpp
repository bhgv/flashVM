/* FlashVM: SWF file parsing and model access: decode.
 * File: src/swf/decode.hpp.
 */

#ifndef FLASHVM_SWF_DECODE_HPP
/* Define FLASHVM SWF DECODE HPP constant or compilation switch. */
#define FLASHVM_SWF_DECODE_HPP
#include "flashvm/swf.h"
#include "../details/vector.hpp"
/* Decode into canonical FWS bytes. The limit is applied before allocation. */
/* Decode swf. */
FVM_Result fvm_swf_decode(const void *, size_t, size_t, fvm_detail::vector<unsigned char> &,
                          FVM_SWFInfo *);
#endif

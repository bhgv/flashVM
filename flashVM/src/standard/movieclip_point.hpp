/* FlashVM: stable local/global point adapters sharing the value-based execution service. */
#ifndef FVM_MOVIECLIP_POINT_HPP
#define FVM_MOVIECLIP_POINT_HPP /* Point adapter declarations guard. */
#include "../core/runtime.hpp"
/* Transform the first point argument from clip-local pixels into physical world pixels. */
FVM_Result fvm_movieclip_local_global(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                      FVM_Value *, void *);
/* Transform the first point argument from physical world pixels into clip-local pixels. */
FVM_Result fvm_movieclip_global_local(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                      FVM_Value *, void *);
#endif

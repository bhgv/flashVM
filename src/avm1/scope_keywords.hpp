/* FlashVM: shared keyword recognition without bytecode or interpreter-stack access. */
#ifndef FVM_AVM1_SCOPE_KEYWORDS_HPP
#define FVM_AVM1_SCOPE_KEYWORDS_HPP /* Private versioned namespace recognition guard. */
#include "../core/runtime.hpp"
#include "scope_keyword_constants.h"
/* Match counted UCS2 text against an ASCII keyword, with optional pinned Flash folding. */
bool fvm_avm1_keyword_name(const fvm_detail::string &, const char *, size_t, bool sensitive);
/* Recognize the namespace fallback, retaining unloaded exact spelling and SWF6 availability. */
bool fvm_avm1_global_keyword(FVM_Context *, const fvm_detail::string &);
/* Read loaded concrete MovieClip root/parent/global intrinsics after ordinary own slots.
 * Unmatched names preserve value; errors preserve both outputs. No callbacks or bytecode access. */
FVM_Result fvm_avm1_movieclip_path_property(FVM_Context *, FVM_Object *, const fvm_detail::string &,
                                            FVM_Value *, int *present);
#endif

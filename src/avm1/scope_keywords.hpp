/* FlashVM: shared keyword recognition without bytecode or interpreter-stack access. */
#ifndef FVM_AVM1_SCOPE_KEYWORDS_HPP
#define FVM_AVM1_SCOPE_KEYWORDS_HPP /* Private versioned namespace recognition guard. */
#include "../core/runtime.hpp"
#include "scope_keyword_constants.h"
/* Match complete counted UCS2 text with optional pinned Flash case folding. */
bool fvm_avm1_keyword_name(const fvm_detail::string &, const char *, size_t, bool sensitive);
/* Recognize the namespace fallback, retaining unloaded exact spelling and SWF6 availability. */
bool fvm_avm1_global_keyword(FVM_Context *, const fvm_detail::string &);
/* Test the concrete MovieClip namespace after callers have checked its ordinary own slots. */
bool fvm_avm1_movieclip_global(FVM_Context *, FVM_Object *, const fvm_detail::string &);
#endif

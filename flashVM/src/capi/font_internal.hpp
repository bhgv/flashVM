/* FlashVM: C embedding API: font internal.
 * File: src/capi/font_internal.hpp.
 */

#ifndef FVM_FONT_INTERNAL_HPP
/* Define FVM FONT INTERNAL HPP constant or compilation switch. */
#define FVM_FONT_INTERNAL_HPP
#ifdef FVM_HAS_FONTS
#include "flashvm/font.h"
#include "hb.h"
/* Shape a script/level segment with surrounding line context. Internal callers
 * have already validated UTF-8 and scalar boundaries. Clusters are line-relative. */
/* Implement font shape segment for this subsystem. */
FVM_Result fvm_font_shape_segment(FVM_Font *, const char *, size_t, size_t, size_t,
                                  FVM_TextDirection, hb_script_t, const char *, FVM_TextRun **);
/* Shape with explicit native features while retaining the surrounding line context. */
FVM_Result fvm_font_shape_segment_configured(FVM_Font *, const char *, size_t, size_t, size_t,
                                  FVM_TextDirection, hb_script_t, const char *, unsigned int,
                                  FVM_TextRun **);
#endif
#endif

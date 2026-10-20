/* Private immutable content records are ordinary managed graphs, portable through serialization. */
#ifndef FVM_MOVIECLIP_CONTENT_HPP
#define FVM_MOVIECLIP_CONTENT_HPP /* Guard content record construction helpers. */
#include "../core/runtime.hpp"
#include "flashvm/movieclip.h"
#define FVM_MOVIECLIP_CONTENT_RECORD_NAME "content_info" /* Private owning state edge. */
/* [GC:] Build a complete immutable record on unpublished content under the caller's barrier.
 * No script lookup/callback; may allocate/throw before the caller publishes the graph. */
FVM_Result fvm_movieclip_record_content(FVM_Context *, FVM_Object *state,
                                        FVM_MovieClipContentFormat, size_t encoded, size_t decoded,
                                        unsigned version, unsigned width, unsigned height);
/* [GC:] Retain the immutable source record when duplicating imported content. */
void fvm_movieclip_copy_content_record(FVM_Object *source, FVM_Object *destination);
#endif

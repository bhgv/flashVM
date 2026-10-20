/* FlashVM: shared native construction helpers with no bytecode or script method lookup. */
#ifndef FVM_MOVIECLIP_CONSTRUCTION_HPP
#define FVM_MOVIECLIP_CONSTRUCTION_HPP /* Private native construction interface guard. */
#include "../core/runtime.hpp"
/* [GC:] Clone editable paint graphs while retaining live texture edges; caller blocks collection.
 */
FVM_Result fvm_movieclip_copy_drawing(FVM_Context *, FVM_Object *source, FVM_Object *destination);
/* [GC:] Resolve class/prototype from current or transported registry under caller's native barrier.
 */
FVM_Result fvm_movieclip_sprite_class(FVM_Context *, FVM_Object *library, const char *linkage,
                                      FVM_Object *&prototype, FVM_Object *&constructor);
/* [GC:] Create a temporary one-frame empty clip with no prototype/global/script lookup.
 * Only its private content state is published by unload; the target prototype stays intact. */
FVM_Result fvm_movieclip_empty_content(FVM_Context *, FVM_Object **out);
/* [GC:] Build a private image rectangle through native drawing helpers, with no script lookup. */
FVM_Result fvm_movieclip_image_content(FVM_Context *, const FVM_Value &, unsigned width,
                                       unsigned height, FVM_Object **out);
#endif

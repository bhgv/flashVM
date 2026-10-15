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
#endif

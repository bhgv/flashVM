/* Shared independent scene fixture for optional MovieClip input methods. */
#include "clip_input_fixture.hpp"
#include "flashvm/function.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
/* Wrap a borrowed managed object without extending its lifetime across host calls. */
static FVM_Value method_value(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}
/* Create a pinned selected root with one placed sprite and an independent record marker. */
static FVM_Object *method_root(FVM_Context *c, unsigned version = 8)
{
    FVM_Object *root = clip_load(c, clip_movie(version, clip_event_table(version, 248, action(42))));
    assert(fvm_movieclip_advance(c, root, 1) == FVM_OK);
    return root;
}
/* The method must observe the corresponding retained record before changing its marker. */
static FVM_Result method_marker(FVM_Context *c, FVM_Object *self, unsigned count,
                                const FVM_Value *, FVM_Value *out, void *)
{
    assert(!count && c->clip_input_dispatching && c->stage_update_allowed);
    FVM_Value value;
    FVM_Result result = fvm_object_get(c, self, "seen", &value);
    if (result != FVM_OK) return result;
    assert(value.type == FVM_VALUE_NUMBER && value.number == 42);
    fvm_value_number(&value, 84);
    result = fvm_object_set(c, self, "seen", &value);
    return result == FVM_OK ? fvm_call(c, "updateAfterEvent", 0, 0, out) : result;
}

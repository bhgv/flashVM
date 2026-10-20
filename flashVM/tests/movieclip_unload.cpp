/* Transactional empty content, preserved identity/handlers/origin and retained detached children. */
#define FVM_LOADING_FIXTURE_ONLY
#include "movieclip_loading.cpp"
#include "flashvm/execution.h"
#include "flashvm/stage.h"
#include "flashvm/clip_actions.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_storage.hpp"
#include "../src/standard/movieclip_state.hpp"
#include "../src/core/native_call_guard.hpp"
static unsigned int unwanted_calls;
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *)
{ ++unwanted_calls; return FVM_ERROR; }
#ifndef FVM_UNLOAD_FIXTURE_ONLY
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0; FVM_Value value, source;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED) == FVM_OK);
    assert(fvm_load_memory(c,movie,sizeof(movie)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_load_swf(c,movie,sizeof(movie),"_root",&root) == FVM_OK);
    assert(fvm_gc_pin(c,root) == FVM_OK);
    assert(fvm_movieclip_set_source(c,root,"http://host/root.swf",20) == FVM_OK);
    FVM_Object *slot = child(c,root,"slot"), *previous = child(c,slot,"previous");
    assert(fvm_gc_pin(c,previous) == FVM_OK);
    int attached = 17;
    assert(fvm_movieclip_attached_to(other,slot,root,&attached) == FVM_INVALID_ARGUMENT && attached == 17);
    assert(fvm_movieclip_attached_to(c,previous,root,&attached) == FVM_OK && attached);
    assert(fvm_movieclip_attached_to(c,root,slot,&attached) == FVM_OK && !attached);
    attached = 17;
    assert(fvm_movieclip_attached_to(c,slot,root,0) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_attached_to(c,0,root,&attached) == FVM_INVALID_ARGUMENT && attached == 17);
    FVM_Object *physical = slot->resource_result.object;
    FVM_Value real_parent = physical->properties["parent"];
    physical->properties["parent"] = fvm_movieclip_storage::object(slot);
    assert(fvm_movieclip_attached_to(c,previous,root,&attached) == FVM_INVALID_ARGUMENT && attached == 17);
    physical->properties["parent"] = real_parent;
    physical->properties["detached"] = fvm_movieclip_storage::number(FVM_MOVIECLIP_DETACHED_STATE);
    assert(fvm_movieclip_attached_to(c,previous,root,&attached) == FVM_OK && !attached);
    physical->properties["detached"] = fvm_movieclip_storage::number(FVM_MOVIECLIP_ATTACHED_STATE);
    assert(fvm_movieclip_attached_to(c,previous,root,&attached) == FVM_OK && attached);

    fvm_value_string(&value,"kept"); assert(fvm_object_set(c,slot,"keep",&value) == FVM_OK);
    assert(fvm_object_define_method_rooted(c,slot,"onUnload",forbidden,0,0) == FVM_OK);
    FVM_Affine matrix = {2,1,.5,3,19,27}, actual;
    assert(fvm_movieclip_set_matrix(c,slot,&matrix) == FVM_OK);
    fvm_value_boolean(&value,1); assert(fvm_object_set(c,slot,"_lockroot",&value) == FVM_OK);
    FVM_Object *old = slot->resource_result.object;
    FVM_Object *events = 0, *record = 0;
    assert(fvm_object_create(c,&events) == FVM_OK && fvm_object_create(c,&record) == FVM_OK);
    old->properties["clip_actions"] = fvm_movieclip_storage::object(events);
    record->properties["events"] = fvm_movieclip_storage::number(FVM_CLIP_EVENT_ENTER_FRAME);
    record->properties["key"] = fvm_movieclip_storage::number(0);
    fvm_value_string(&value,"00"); record->indexed[0] = value;
    events->indexed[0] = fvm_movieclip_storage::object(record);
    FVM_Value args[2];
    fvm_value_number(&args[0],0xff0000); fvm_value_number(&args[1],100);
    assert(fvm_object_call(c,slot,"beginFill",2,args,&value) == FVM_OK);
    fvm_value_number(&args[0],0); fvm_value_number(&args[1],0);
    assert(fvm_object_call(c,slot,"moveTo",2,args,&value) == FVM_OK);
    fvm_value_number(&args[0],10); assert(fvm_object_call(c,slot,"lineTo",2,args,&value) == FVM_OK);
    fvm_value_number(&args[0],0); fvm_value_number(&args[1],10);
    assert(fvm_object_call(c,slot,"lineTo",2,args,&value) == FVM_OK);
    fvm_value_number(&args[1],0); assert(fvm_object_call(c,slot,"lineTo",2,args,&value) == FVM_OK);
    assert(fvm_object_call(c,slot,"endFill",0,0,&value) == FVM_OK);
    unsigned char pixels[64*64*4]; std::memset(pixels,0,sizeof(pixels));
    FVM_RasterSurface surface; surface.pixels = pixels; surface.width = 64; surface.height = 64; surface.stride = 64*4;
    assert(fvm_movieclip_render(c,slot,&surface) == FVM_OK);
    bool painted = false; for (size_t i=0;i<sizeof(pixels);++i) if(pixels[i]) painted = true;
    assert(painted);
    FVM_Object *old_frames = old->properties["frames"].object;
    fvm_value_number(&value,99); old_frames->indexed[0] = value;
    assert(fvm_movieclip_unload(other,slot) == FVM_INVALID_ARGUMENT);
    {
        FVM_NativeCallGuard guard(c);
        assert(fvm_movieclip_unload(c,slot) == FVM_INVALID_ARGUMENT);
    }
    assert(fvm_movieclip_unload(c,0) == FVM_INVALID_ARGUMENT && !unwanted_calls);
    assert(fvm_movieclip_unload(c,slot) == FVM_OK && !unwanted_calls);
    assert(slot->resource_result.object != old);
    FVM_Object *state = slot->resource_result.object;
    assert(state->properties["frames"].object->indexed.empty());
    assert(state->properties["paths"].object->indexed.empty());
    assert(state->properties["clip_actions"].object == events);
    std::memset(pixels,0,sizeof(pixels)); assert(fvm_movieclip_render(c,slot,&surface) == FVM_OK);
    for (size_t i=0;i<sizeof(pixels);++i) assert(!pixels[i]);
    assert(fvm_movieclip_dispatch_clip_actions(c,slot,FVM_CLIP_EVENT_ENTER_FRAME,0) == FVM_OK);
    assert(state->properties["children"].object->indexed.empty());
    assert(state->properties["total"].number == 1 && state->properties["frame"].number == 1);
    assert(state->properties["playing"].number == 0);
    assert(fvm_object_get(c,root,"slot",&value) == FVM_OK && value.object == slot);
    assert(fvm_object_get(c,slot,"previous",&value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    assert(fvm_exec_clip_detached(c,previous,&value) == FVM_OK && value.boolean);
    assert(fvm_movieclip_attached_to(c,previous,root,&attached) == FVM_OK && !attached);
    assert(fvm_movieclip_attached_to(c,slot,root,&attached) == FVM_OK && attached);
    assert(fvm_movieclip_get_source(c,previous,&source) == FVM_OK && fvm_value_string_size(&source) == 20);
    assert(fvm_object_get(c,slot,"_lockroot",&value) == FVM_OK && value.boolean);
    assert(fvm_movieclip_get_matrix(c,slot,&actual) == FVM_OK && actual.xx == matrix.xx && actual.x0 == matrix.x0);
    text(c,slot,"keep","kept",4);
    assert(fvm_gc_collect(c,0) == FVM_OK);
    assert(fvm_movieclip_advance(c,root,1) == FVM_OK);
    assert(fvm_movieclip_unload(c,slot) == FVM_OK && !unwanted_calls);
    /* Replaceable global/prototype lookup must not run during empty-content construction. */
    FVM_Value klass; assert(fvm_get_global(c,"MovieClip",&klass) == FVM_OK);
    klass.object->properties.erase("prototype");
    assert(fvm_object_define_accessor_rooted(c,klass.object,"prototype",forbidden,0,0,0) == FVM_OK);
    assert(fvm_movieclip_unload(c,slot) == FVM_OK && !unwanted_calls);
    assert(fvm_gc_collect(c,0) == FVM_OK);
    assert(fvm_gc_unpin(c,previous) == FVM_OK && fvm_gc_unpin(c,root) == FVM_OK);
    fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(runtime);
    return 0;
}
#endif

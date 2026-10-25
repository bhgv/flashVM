/* Transactional empty content, preserved identity/handlers/origin and retained detached children. */
#define FVM_LOADING_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
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
{
    ++unwanted_calls;
    return FVM_ERROR;
}
#ifndef FVM_UNLOAD_FIXTURE_ONLY


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0;
    FVM_Value value, source;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &c) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_load_swf(c, movie, sizeof(movie), "_root", &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_source(c, root, "http://host/root.swf", 20) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *slot = child(c, root, "slot"), *previous = child(c, slot, "previous");
    if (!(fvm_gc_pin(c, previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int attached = 17;
    if (!(fvm_movieclip_attached_to(other, slot, root, &attached) == FVM_INVALID_ARGUMENT && attached == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_attached_to(c, previous, root, &attached) == FVM_OK && attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_attached_to(c, root, slot, &attached) == FVM_OK && !attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    attached = 17;
    if (!(fvm_movieclip_attached_to(c, slot, root, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_attached_to(c, 0, root, &attached) == FVM_INVALID_ARGUMENT && attached == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *physical = slot->resource_result.object;
    FVM_Value real_parent = physical->properties["parent"];
    physical->properties["parent"] = fvm_movieclip_storage::object(slot);
    if (!(fvm_movieclip_attached_to(c, previous, root, &attached) == FVM_INVALID_ARGUMENT && attached == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    physical->properties["parent"] = real_parent;
    physical->properties["detached"] = fvm_movieclip_storage::number(FVM_MOVIECLIP_DETACHED_STATE);
    if (!(fvm_movieclip_attached_to(c, previous, root, &attached) == FVM_OK && !attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    physical->properties["detached"] = fvm_movieclip_storage::number(FVM_MOVIECLIP_ATTACHED_STATE);
    if (!(fvm_movieclip_attached_to(c, previous, root, &attached) == FVM_OK && attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    fvm_value_string(&value, "kept");
    if (!(fvm_object_set(c, slot, "keep", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, slot, "onUnload", forbidden, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Affine matrix = {2, 1, .5, 3, 19, 27}, actual;
    if (!(fvm_movieclip_set_matrix(c, slot, &matrix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(c, slot, "_lockroot", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *old = slot->resource_result.object;
    FVM_Object *events = 0, *record = 0;
    if (!(fvm_object_create(c, &events) == FVM_OK && fvm_object_create(c, &record) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    old->properties["clip_actions"] = fvm_movieclip_storage::object(events);
    record->properties["events"] = fvm_movieclip_storage::number(FVM_CLIP_EVENT_ENTER_FRAME);
    record->properties["key"] = fvm_movieclip_storage::number(0);
    fvm_value_string(&value, "00");
    record->indexed[0] = value;
    events->indexed[0] = fvm_movieclip_storage::object(record);
    FVM_Value args[2];
    fvm_value_number(&args[0], 0xff0000);
    fvm_value_number(&args[1], 100);
    if (!(fvm_object_call(c, slot, "beginFill", 2, args, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 0);
    fvm_value_number(&args[1], 0);
    if (!(fvm_object_call(c, slot, "moveTo", 2, args, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 10);
    if (!(fvm_object_call(c, slot, "lineTo", 2, args, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 0);
    fvm_value_number(&args[1], 10);
    if (!(fvm_object_call(c, slot, "lineTo", 2, args, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[1], 0);
    if (!(fvm_object_call(c, slot, "lineTo", 2, args, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, slot, "endFill", 0, 0, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char pixels[64 * 64 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface;
    surface.pixels = pixels;
    surface.width = 64;
    surface.height = 64;
    surface.stride = 64 * 4;
    if (!(fvm_movieclip_render(c, slot, &surface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool painted = false;
    for (size_t i = 0; i < sizeof(pixels); ++i)
    {
        if (pixels[i])
        {
            painted = true;
        }
    }
    if (!(painted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *old_frames = old->properties["frames"].object;
    fvm_value_number(&value, 99);
    old_frames->indexed[0] = value;
    if (!(fvm_movieclip_unload(other, slot) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {


        FVM_NativeCallGuard guard(c);
        if (!(fvm_movieclip_unload(c, slot) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_unload(c, 0) == FVM_INVALID_ARGUMENT && !unwanted_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(c, slot) == FVM_OK && !unwanted_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(slot->resource_result.object != old))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *state = slot->resource_result.object;
    if (!(state->properties["frames"].object->indexed.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state->properties["paths"].object->indexed.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state->properties["clip_actions"].object == events))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memset(pixels, 0, sizeof(pixels));
    if (!(fvm_movieclip_render(c, slot, &surface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < sizeof(pixels); ++i)
    {
        if (!(!pixels[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_dispatch_clip_actions(c, slot, FVM_CLIP_EVENT_ENTER_FRAME, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state->properties["children"].object->indexed.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state->properties["total"].number == 1 && state->properties["frame"].number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state->properties["playing"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, root, "slot", &value) == FVM_OK && value.object == slot))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, slot, "previous", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_clip_detached(c, previous, &value) == FVM_OK && value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_attached_to(c, previous, root, &attached) == FVM_OK && !attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_attached_to(c, slot, root, &attached) == FVM_OK && attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_source(c, previous, &source) == FVM_OK && fvm_value_string_size(&source) == 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, slot, "_lockroot", &value) == FVM_OK && value.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_matrix(c, slot, &actual) == FVM_OK && actual.xx == matrix.xx && actual.x0 == matrix.x0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    text(c, slot, "keep", "kept", 4);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(c, slot) == FVM_OK && !unwanted_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Replaceable global/prototype lookup must not run during empty-content construction. */
    FVM_Value klass;
    if (!(fvm_get_global(c, "MovieClip", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    klass.object->properties.erase("prototype");
    if (!(fvm_object_define_accessor_rooted(c, klass.object, "prototype", forbidden, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(c, slot) == FVM_OK && !unwanted_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, previous) == FVM_OK && fvm_gc_unpin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
#endif

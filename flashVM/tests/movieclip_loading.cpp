/* Same-context replacement preserves identity/placement; form decode is counted and bounded. */
#include "flashvm/flashvm.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>
#include <cmath>
static const unsigned char movie[] = {70, 87, 83, 8, 25, 0,  0, 0, 128, 0, 0, 20, 0,
                                      0,  0,  20, 0, 0,  24, 1, 0, 64,  0, 0, 0};
/* Create a native child using the same public MovieClip method available to SWF code. */
static FVM_Object *child(FVM_Context *c, FVM_Object *parent, const char *name)
{
    FVM_Value args[2], result;
    fvm_value_string(&args[0], name);
    fvm_value_number(&args[1], 1);
    assert(fvm_object_call(c, parent, "createEmptyMovieClip", 2, args, &result) == FVM_OK);
    assert(result.type == FVM_VALUE_OBJECT && result.object);
    return result.object;
}
/* Assert managed counted values without relying on terminating NUL. */
static void text(FVM_Context *c, FVM_Object *o, const char *key, const char *expected, size_t size)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, key, &v) == FVM_OK && v.type == FVM_VALUE_STRING);
    assert(fvm_value_string_size(&v) == size && !std::memcmp(v.string, expected, size));
}
static unsigned int notifications;
/* Completion callbacks observe fully assigned form values. */
static FVM_Result on_data(FVM_Context *c, FVM_Object *target, unsigned int, const FVM_Value *,
                          FVM_Value *result, void *)
{
    assert(fvm_movieclip_replace_swf(c, target, movie, sizeof(movie), "nested.swf", 10) ==
           FVM_INVALID_ARGUMENT);
    ++notifications;
    fvm_value_undefined(result);
    return FVM_OK;
}
#ifndef FVM_LOADING_FIXTURE_ONLY
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0;
    FVM_Value value, source;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
    assert(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_load_swf(c, movie, sizeof(movie), "_root", &root) == FVM_OK);
    assert(fvm_gc_pin(c, root) == FVM_OK);
    assert(fvm_movieclip_set_source(c, root, "/exe/start.swf", 14) == FVM_OK);
    FVM_Object *slot = child(c, root, "slot"), *previous = child(c, slot, "previous");
    assert(fvm_gc_pin(c, previous) == FVM_OK);
    assert(fvm_movieclip_get_source(c, slot, &source) == FVM_OK &&
           fvm_value_string_size(&source) == 14);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = previous;
    assert(fvm_object_set(c, slot, "saved", &value) == FVM_OK);
    FVM_Affine matrix = {2, 1, .5, 3, 19, 27}, actual;
    assert(fvm_movieclip_set_matrix(c, slot, &matrix) == FVM_OK);
    fvm_value_string(&value, "retained");
    assert(fvm_object_set(c, slot, "keep", &value) == FVM_OK);
    assert(fvm_movieclip_replace_swf(c, slot, "bad", 3, "http://host/a/child.swf", 23) != FVM_OK);
    assert(fvm_object_get(c, slot, "previous", &value) == FVM_OK && value.object == previous);
    const char *location = "http://host/a/child.swf";
    assert(fvm_movieclip_replace_swf(c, slot, movie, sizeof(movie), location,
                                     std::strlen(location)) == FVM_OK);
    assert(fvm_object_get(c, root, "slot", &value) == FVM_OK && value.object == slot);
    assert(fvm_movieclip_get_matrix(c, slot, &actual) == FVM_OK);
    assert(std::fabs(actual.xx - matrix.xx) < 1e-12 && std::fabs(actual.xy - matrix.xy) < 1e-12);
    assert(actual.x0 == matrix.x0 && actual.y0 == matrix.y0);
    text(c, slot, "keep", "retained", 8);
    assert(fvm_object_get(c, slot, "saved", &value) == FVM_OK && value.object == previous);
    assert(fvm_movieclip_get_source(c, slot, &source) == FVM_OK);
    assert(fvm_value_string_size(&source) == std::strlen(location));
    assert(!std::memcmp(source.string, location, std::strlen(location)));
    assert(fvm_object_get(c, slot, "previous", &value) == FVM_OK &&
           value.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_get(c, previous, "_parent", &value) == FVM_OK);
    assert(fvm_movieclip_get_source(c, previous, &source) == FVM_OK);
    assert(fvm_value_string_size(&source) == 14 &&
           !std::memcmp(source.string, "/exe/start.swf", 14));
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, slot, "onData", on_data, 0, 0) == FVM_OK);
    const char form[] =
        "hello=world+space&equal=a%3Db%26c&zero=a%00b&duplicate=first&duplicate=last&empty=";
    assert(fvm_object_load_variables(c, slot, form, sizeof(form) - 1) == FVM_OK);
    assert(notifications == 1);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    text(c, slot, "hello", "world space", 11);
    text(c, slot, "equal", "a=b&c", 5);
    text(c, slot, "zero", "a\0b", 3);
    text(c, slot, "duplicate", "last", 4);
    text(c, slot, "empty", "", 0);
    const char bad[] = "hello=changed&bad=%Q0";
    assert(fvm_object_load_variables(c, slot, bad, sizeof(bad) - 1) == FVM_INVALID_ARGUMENT);
    text(c, slot, "hello", "world space", 11);
    assert(notifications == 1);
    assert(fvm_object_load_variables(c, slot, 0, 0) == FVM_OK);
    assert(notifications == 2);
    char *segments = new char[FVM_FORM_MAX_FIELDS + 1];
    std::memset(segments, '&', FVM_FORM_MAX_FIELDS + 1);
    assert(fvm_object_load_variables(c, slot, segments, FVM_FORM_MAX_FIELDS + 1) ==
           FVM_UNSUPPORTED);
    delete[] segments;
    assert(notifications == 2);
    assert(fvm_movieclip_advance(c, root, 1) == FVM_OK);
    assert(fvm_gc_unpin(c, previous) == FVM_OK);
    assert(fvm_gc_unpin(c, root) == FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}

#endif

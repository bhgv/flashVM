/* Same-context replacement preserves identity/placement; form decode is counted and bounded. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/flashvm.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
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
    if (!(fvm_object_call(c, parent, "createEmptyMovieClip", 2, args, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.type == FVM_VALUE_OBJECT && result.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return result.object;
}


/* Assert managed counted values without relying on terminating NUL. */
static void text(FVM_Context *c, FVM_Object *o, const char *key, const char *expected, size_t size)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, key, &v) == FVM_OK && v.type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(&v) == size && !std::memcmp(v.string, expected, size)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}

static unsigned int notifications;


/* Completion callbacks observe fully assigned form values. */
static FVM_Result
on_data(FVM_Context *c, FVM_Object *target, unsigned int, const FVM_Value *, FVM_Value *result, void *)
{
    if (!(fvm_movieclip_replace_swf(c, target, movie, sizeof(movie), "nested.swf", 10) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
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
    if (!(fvm_movieclip_set_source(c, root, "/exe/start.swf", 14) == FVM_OK))
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
    if (!(fvm_movieclip_get_source(c, slot, &source) == FVM_OK && fvm_value_string_size(&source) == 14))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = previous;
    if (!(fvm_object_set(c, slot, "saved", &value) == FVM_OK))
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
    fvm_value_string(&value, "retained");
    if (!(fvm_object_set(c, slot, "keep", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_swf(c, slot, "bad", 3, "http://host/a/child.swf", 23) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, slot, "previous", &value) == FVM_OK && value.object == previous))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *location = "http://host/a/child.swf";
    if (!(fvm_movieclip_replace_swf(c, slot, movie, sizeof(movie), location, std::strlen(location)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, root, "slot", &value) == FVM_OK && value.object == slot))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_matrix(c, slot, &actual) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fabs(actual.xx - matrix.xx) < 1e-12 && std::fabs(actual.xy - matrix.xy) < 1e-12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(actual.x0 == matrix.x0 && actual.y0 == matrix.y0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    text(c, slot, "keep", "retained", 8);
    if (!(fvm_object_get(c, slot, "saved", &value) == FVM_OK && value.object == previous))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_source(c, slot, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(&source) == std::strlen(location)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(source.string, location, std::strlen(location))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, slot, "previous", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, previous, "_parent", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_source(c, previous, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(&source) == 14 && !std::memcmp(source.string, "/exe/start.swf", 14)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, slot, "onData", on_data, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char form[] = "hello=world+space&equal=a%3Db%26c&zero=a%00b&duplicate=first&duplicate=last&empty=";
    if (!(fvm_object_load_variables(c, slot, form, sizeof(form) - 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(notifications == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    text(c, slot, "hello", "world space", 11);
    text(c, slot, "equal", "a=b&c", 5);
    text(c, slot, "zero", "a\0b", 3);
    text(c, slot, "duplicate", "last", 4);
    text(c, slot, "empty", "", 0);
    const char bad[] = "hello=changed&bad=%Q0";
    if (!(fvm_object_load_variables(c, slot, bad, sizeof(bad) - 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    text(c, slot, "hello", "world space", 11);
    if (!(notifications == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_load_variables(c, slot, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(notifications == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    char *segments = new char[FVM_FORM_MAX_FIELDS + 1];
    std::memset(segments, '&', FVM_FORM_MAX_FIELDS + 1);
    if (!(fvm_object_load_variables(c, slot, segments, FVM_FORM_MAX_FIELDS + 1) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    delete[] segments;
    if (!(notifications == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}

#endif

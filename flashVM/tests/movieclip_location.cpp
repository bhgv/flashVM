/* Readonly named/numeric URL properties track copied origins rather than script shadows. */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include "avm1_target_scope.cpp"
#include "flashvm/serialization.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
#include "../src/standard/movieclip_location.hpp"
static unsigned coercions;
/* Readonly assignment must not invoke input conversion. */
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *,
                            void *)
{
    ++coercions;
    return FVM_ERROR;
}
/* Verify an entire counted string, without accepting a matching prefix. */
static void source(FVM_Context *c, FVM_Object *clip, const char *wanted)
{
    FVM_Value out;
    assert(fvm_object_get(c, clip, "_url", &out) == FVM_OK && out.type == FVM_VALUE_STRING);
    assert(fvm_value_string_size(&out) == std::strlen(wanted) &&
           !std::memcmp(out.string, wanted, std::strlen(wanted)));
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                   fvm_standard_install(c) == FVM_OK);
            assert(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER,
                                          0) == FVM_OK);
            assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                   fvm_gc_pin(c, root) == FVM_OK);
            source(c, root, "");
            const char *old = "https://host.example/old/root.swf";
            assert(fvm_movieclip_set_source(c, root, old, std::strlen(old)) == FVM_OK);
            FVM_Object *clip = child(c, root, "child", 1);
            assert(fvm_gc_pin(c, clip) == FVM_OK);
            source(c, clip, old);
            FVM_Value cv, out, index, input;
            fvm_value_undefined(&cv);
            cv.type = FVM_VALUE_OBJECT;
            cv.object = clip;
            fvm_value_number(&index, 15);
            assert(fvm_exec_display_property(c, 0, &cv, &index, 0, &out) == FVM_OK &&
                   !std::strcmp(out.string, old));
            FVM_Object *coercer = 0;
            assert(fvm_object_create(c, &coercer) == FVM_OK);
            assert(fvm_object_define_method_rooted(c, coercer, "toString", forbidden, 0, 0) ==
                   FVM_OK);
            assert(fvm_object_define_method_rooted(c, coercer, "valueOf", forbidden, 0, 0) ==
                   FVM_OK);
            fvm_value_undefined(&input);
            input.type = FVM_VALUE_OBJECT;
            input.object = coercer;
            coercions = 0;
            assert(fvm_object_set(c, clip, "_url", &input) == FVM_OK);
            assert(fvm_exec_display_property(c, 1, &cv, &index, &input, &out) == FVM_OK &&
                   coercions == 0);
            source(c, clip, old);
            assert(fvm_set_global(c, "located", &cv) == FVM_OK);
            Bytes actions;
            text(actions, "namedLocation");
            global(actions, "located");
            text(actions, "_url");
            actions.push_back(0x4e);
            actions.push_back(0x1d);
            assert(execute(c, root, actions) == FVM_OK);
            FVM_ExecutionStats stats;
            stats.struct_size = sizeof(stats);
            assert(fvm_execution_get_stats(c, &stats) == FVM_OK);
            if (mode && fvm_jit_available())
                assert(stats.native_entries > 0);
            else
                assert(stats.interpreter_entries > 0);
            assert(fvm_object_get(c, root, "namedLocation", &out) == FVM_OK &&
                   !std::strcmp(out.string, old));
            /* Unload detaches the child with its old origin before the root source changes. */
            assert(fvm_movieclip_unload(c, root) == FVM_OK);
            const char *next = "https://host.example/final/root.swf";
            assert(fvm_movieclip_set_source(c, root, next, std::strlen(next)) == FVM_OK);
            source(c, root, next);
            source(c, clip, old);
            assert(fvm_movieclip_set_source(c, clip, "bad\0tail", 8) == FVM_INVALID_ARGUMENT);
            source(c, clip, old);
            /* Content replacement changes the location only when the new content is published. */
            assert(fvm_movieclip_replace_swf(c, clip, swf, sizeof(swf), "/new.swf", 8) !=
                   FVM_OK); /* Malformed frame count. */
            source(c, clip, old);
            const unsigned char valid[] = {70, 87, 83, 8, 25, 0,  0, 0, 128, 0, 0, 20, 0,
                                           0,  0,  20, 0, 0,  24, 1, 0, 64,  0, 0, 0};
            assert(fvm_movieclip_replace_swf(c, clip, valid, sizeof(valid), "/new.swf", 8) ==
                   FVM_OK);
            source(c, clip, "/new.swf");
            assert(fvm_movieclip_unload(c, clip) == FVM_OK);
            source(c, clip, "/new.swf");
            FVM_Context *other = 0;
            assert(fvm_context_create(rt, &other) == FVM_OK &&
                   fvm_standard_install(other) == FVM_OK);
            unsigned char *wire = 0;
            size_t size = 0;
            assert(fvm_value_serialize(c, &cv, &wire, &size) == FVM_OK);
            assert(fvm_value_deserialize(other, wire, size, &out) == FVM_OK);
            fvm_serialized_free(wire);
            FVM_Object *copy = out.object;
            assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
            source(other, copy, "/new.swf");
            assert(fvm_movieclip_set_source(other, copy, "/copy.swf", 9) == FVM_OK);
            source(other, copy, "/copy.swf");
            source(c, clip, "/new.swf");
            fvm_context_destroy(other);
            unsigned long id = fvm_movieclip_callback_id(fvm_movieclip_url_get);
            assert(id && fvm_movieclip_callback_from_id(id) == fvm_movieclip_url_get);
            /* A raw user data shadow cannot change the numeric intrinsic origin. */
            fvm_value_string(&input, "shadow");
            clip->properties["_url"] = input;
            source(c, clip, "shadow");
            assert(fvm_exec_display_property(c, 0, &cv, &index, 0, &out) == FVM_OK &&
                   !std::strcmp(out.string, "/new.swf"));
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            fvm_context_destroy(c);
        }
    fvm_runtime_destroy(rt);
    return 0;
}

/* Projected watcher Functions remain callable, rooted and transferable. */
#include "watch_boxing_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned mode = 0; mode < 5; ++mode)
    for (unsigned route = 0; route < 5; ++route)
    {
        if ((version && version < 4) || (route >= 3 && !fvm_jit_available())) continue;
        WatchBoxingFixture f; watch_box_init(rt, f, version, type, mode);
        FVM_Context *c = f.context;
        FVM_Value args[3], out; assert(fvm_value_string_bytes(args, "watched\0x", 9) == FVM_OK);
        args[1] = f.input; fvm_value_number(args + 2, 7);
        assert(fvm_set_global(c, "watchName", args) == FVM_OK);
        FVM_Result expected = !version || mode < 3 ? FVM_OK : mode == 3 ? FVM_ERROR : FVM_EXCEPTION;
        if (!route) assert(fvm_object_call(c, f.target, "watch", 3, args, &out) == expected);
        else if (route == 1) {
            FVM_Value method; assert(fvm_object_get(c, f.target, "watch", &method) == FVM_OK && fvm_function_invoke(c, method.object, f.target, 3, args, &out) == expected);
        }
        else {
            Program code; code.number(7); code.variable("watchInput"); code.variable("watchName"); code.number(3); code.variable("watchTarget"); code.text("watch"); code.op(0x52); code.finish();
            out = boxing_run(c, code, route - 2, expected);
        }
        bool installed = version && mode == 0;
        if (expected == FVM_OK) assert(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == installed);
        if (expected == FVM_EXCEPTION && route < 2) assert(c->exception_pending && c->exception_value.object == f.target && fvm_exception_clear(c) == FVM_OK);
        assert(f.calls == (version ? 1U : 0U) && f.target->watchers.empty() == !installed);
        if (installed) {
            const char *names[] = {"Number", "Boolean", "String"};
            FVM_Value missing; fvm_value_undefined(&missing);
            assert(fvm_set_global(c, names[type], &missing) == FVM_OK);
        }
        FVM_Value input; fvm_value_number(&input, 5);
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_set_bytes(c, f.target, "watched\0x", 9, &input) == FVM_OK && fvm_object_get_bytes(c, f.target, "watched\0x", 9, &out) == FVM_OK && out.number == (installed ? 12 : 5));
        if (installed) {
            assert(fvm_object_get(c, f.target, "seen", &out) == FVM_OK && out.number == 7);
            unsigned char *data = 0; size_t size = 0; out = class_value(f.target);
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK && fvm_value_serialize(c, &out, &data, &size) == FVM_OK && fvm_value_deserialize(other, data, size, &out) == FVM_OK);
            fvm_serialized_free(data);
            FVM_Object *copy = out.object;
            assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
            fvm_value_number(&input, 6);
            assert(fvm_object_set_bytes(other, copy, "watched\0x", 9, &input) == FVM_OK && fvm_object_get_bytes(other, copy, "watched\0x", 9, &out) == FVM_OK && out.number == 13);
            fvm_context_destroy(other);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

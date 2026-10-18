/* Native SetMember retains thrown watcher graphs across own/inherited collecting setters. */
#define main object_watch_throw_fixture_main
#include "object_watch_throw.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) for (unsigned kind = 0; kind < 6; ++kind) for (setter_throws = 0; setter_throws < 2; ++setter_throws) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *owner = 0, *watch = 0, *get = 0, *set = 0; FVM_Value value, args[3], out;
        const char *key = kind == 1 || kind == 2 ? "0" : "slot";
        assert(fvm_construct(c, kind == 1 ? "Array" : "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        owner = object;
        if (kind == 4) { assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_object_set_prototype(c, object, owner) == FVM_OK); }
        fvm_value_number(&value, 9); assert(fvm_object_set(c, owner, key, &value) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK && fvm_function_set_body(c, watch, throwing_watch, 0, 0) == FVM_OK);
        if (kind == 3 || kind == 4) {
            assert(fvm_gc_pin(c, watch) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, throw_get, 0, 0) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, throw_set, 0, 0) == FVM_OK);
            fvm_value_string(args, key); args[1] = thrown_object(get); args[2] = thrown_object(set);
            assert(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        }
        if (kind == 5) assert(fvm_object_set_property_flags(c, object, key, 4, 0) == FVM_OK);
        fvm_value_string(args, key); args[1] = thrown_object(watch);
        assert(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean);
        setter_calls = 0; fvm_value_number(&value, 81);

        value = thrown_object(object); assert(fvm_set_global(c, "caseObject", &value) == FVM_OK);
        Program p; p.variable("caseObject"); p.text(key); p.number(81); p.op(0x4f); p.number(99); p.finish();
        const bool consumed = kind == 4 && !setter_throws;
        out = class_run(c, p, consumed ? FVM_OK : FVM_EXCEPTION);
        if (consumed) assert(out.number == 99);
        assert(setter_calls == (kind == 3 || kind == 4 ? 1U : 0U));
        if (kind == 3 || kind == 4) {
            assert(fvm_base_accessor_data(owner->accessors.find(fvm_detail::string(key))->second, out));
            if (kind == 3 && !setter_throws) assert(out.type == FVM_VALUE_UNDEFINED); else assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
        } else {
            assert(fvm_object_get(c, object, key, &out) == FVM_OK);
            if (kind == 5) assert(out.number == 9); else assert(out.type == FVM_VALUE_UNDEFINED);
        }
        assert(!c->exception_pending && !c->gc_frames && !c->gc_native_depth && c->watch_running.empty());
        assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

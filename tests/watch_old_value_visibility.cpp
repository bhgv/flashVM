/* Watcher old values skip version-hidden storage without invoking getters. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
static double observed;
static unsigned calls;
static FVM_Result observe(FVM_Context *, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    assert(frame->argc == 4); ++calls;
    observed = frame->argv[1].type == FVM_VALUE_UNDEFINED ? -1 : frame->argv[1].number;
    *out = frame->argv[2]; return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned indexed = 0; indexed < 4; ++indexed)
    for (unsigned virtual_slot = 0; virtual_slot < 2; ++virtual_slot)
    for (unsigned inherited = 0; inherited < 2; ++inherited)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *parent = 0, *function = 0; assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        const char *keys[] = {"slot", "5", "n\0m", "SlOt"}; const size_t sizes[] = {4, 1, 3, 4};
        const char *key = keys[indexed]; const size_t size = sizes[indexed]; FVM_Value value, args[2], out;
        if (virtual_slot) { callback_super_property(c, receiver, key, size, version_function(c, 99), version_function(c, 0)); callback_super_property(c, parent, key, size, version_function(c, 99), version_function(c, 0)); }
        fvm_value_number(&value, 11); assert(fvm_object_set_bytes(c, receiver, key, size, &value) == FVM_OK);
        if (inherited) { fvm_value_number(&value, 22); assert(fvm_object_set_bytes(c, parent, key, size, &value) == FVM_OK); }
        assert(fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
        receiver->attributes[fvm_detail::string(key, size)] = version_masks[version - 4];
        function = version_function(c, 0); assert(fvm_function_set_body(c, function, observe, 0, 0) == FVM_OK);
        fvm_value_string_bytes(args, key, size); args[1] = class_value(function); assert(fvm_object_call(c, receiver, "watch", 2, args, &out) == FVM_OK && out.boolean);
        value = class_value(receiver); assert(fvm_set_global(c, "visibleWatchTarget", &value) == FVM_OK); assert(fvm_set_global(c, "visibleWatchKey", args) == FVM_OK); calls = 0; observed = -99;
        if (!route) { fvm_value_number(&value, 81); assert(fvm_object_set_bytes(c, receiver, key, size, &value) == FVM_OK); }
        else { Program code; code.variable("visibleWatchTarget"); code.variable("visibleWatchKey"); code.number(81); code.op(0x4f); code.number(1); code.finish(); boxing_run(c, code, route - 1, FVM_OK); }
        assert(calls == 1 && observed == (inherited ? 22 : -1));
        if (!virtual_slot) assert(fvm_object_get_bytes(c, receiver, key, size, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 81);
        else { assert(fvm_base_accessor_data(receiver->accessors.find(fvm_detail::string(key, size))->second, out)); assert(out.type == FVM_VALUE_NUMBER && out.number == 81); }
        assert(!c->gc_frames && c->watch_running.empty()); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

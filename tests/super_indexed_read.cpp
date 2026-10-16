/* Super reads select visible indexed ancestors, keeping receiver slots and alias keys separate. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include "super_property_write.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"0", "5", "2147483647", "4294967295", "+5", "05", "n\0m"};
    const size_t sizes[] = {1,1,10,10,2,2,3};
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array)
    for (unsigned key = 0; key < 7; ++key)
    for (unsigned hidden = 0; hidden < 2; ++hidden)
    for (unsigned route = 0; route < 4; ++route) {
        if ((version && version < 4) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *middle = 0, *far = 0;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &middle) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        FVM_Value value, out; fvm_value_number(&value, 91); assert(fvm_object_set_bytes(c, far, keys[key], sizes[key], &value) == FVM_OK);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK);
        fvm_value_number(&value, 777); assert(fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value) == FVM_OK);
        if (hidden && version) middle->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4];
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, middle, far) == FVM_OK && fvm_object_set_prototype(c, receiver, middle) == FVM_OK);
        FVM_Object *facade = super_write_escape(c, receiver); value = class_value(facade); assert(fvm_set_global(c, "superReadTarget", &value) == FVM_OK);
        fvm_value_string_bytes(&value, keys[key], sizes[key]); assert(fvm_set_global(c, "superReadKey", &value) == FVM_OK);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        if (!route) assert(fvm_object_get_bytes(c, facade, keys[key], sizes[key], &out) == FVM_OK);
        else { Program code; code.variable("superReadTarget"); code.variable("superReadKey"); code.op(0x4e); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        if (hidden && version) assert(out.type == FVM_VALUE_NUMBER && out.number == 91);
        else assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
        assert(fvm_object_get_bytes(c, receiver, keys[key], sizes[key], &out) == FVM_OK && out.number == 777);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

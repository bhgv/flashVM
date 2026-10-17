/* Native-method length slots use their stable Function data and legacy name policy. */
#define FVM_INDEX_LIVE_LENGTH_FIXTURE_ONLY
#include "array_index_live_length.cpp"
static FVM_Object *method_length_array(FVM_Context *c, unsigned kind, bool warm, FVM_Object **wrapper = 0)
{
    FVM_Object *array = deleted_length_array(c), *owner = array;
    if (kind == 2) { FVM_Object *old = array->prototype; assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_object_set_prototype(c, owner, old) == FVM_OK && fvm_object_set_prototype(c, array, owner) == FVM_OK); }
    FVM_Value array_class; assert(fvm_get_global(c, "Array", &array_class) == FVM_OK);
    FVM_Value proto; assert(fvm_object_get(c, array_class.object, "prototype", &proto) == FVM_OK);
    FVM_NativeEntry entry = proto.object->methods.find("join")->second;
    const char *name = kind == 1 ? "LeNgTh" : "length";
    assert(fvm_object_define_method_rooted(c, owner, name, entry.callback, entry.userdata, entry.root) == FVM_OK);
    if (warm) {
        FVM_Value value, function = class_value(version_function(c, 1)); assert(fvm_object_get(c, owner, name, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
        if (wrapper) *wrapper = value.object;
        assert(fvm_object_set(c, value.object, "valueOf", &function) == FVM_OK);
    }
    return array;
}
#ifndef FVM_METHOD_LENGTH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"join", "slice", "push"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned warm = 0; warm < 2; ++warm)
    for (unsigned method = 0; method < 5; ++method)
    for (unsigned route = 0; route < 4; ++route)
    for (unsigned transport = 0; transport < 2; ++transport) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0, *source = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = method_length_array(c, kind, !!warm);
        if (transport) {
            FVM_Value value = class_value(array); unsigned char *wire = 0; size_t bytes = 0; source = c;
            assert(fvm_value_serialize(source, &value, &wire, &bytes) == FVM_OK && fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_value_deserialize(c, wire, bytes, &value) == FVM_OK); fvm_serialized_free(wire); array = value.object;
        }
        assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        const unsigned count = warm && (kind == 0 || (kind == 1 && version < 7)) ? 1 : 0; FVM_Value out, value;
        if (method < 3) out = array_deleted_call(c, array, methods[method], route);
        else { fvm_value_number(&value, 81); assert(index_live_write(c, array, method == 3 ? "0" : "3", value, route, FVM_OK) == FVM_OK); }
        if (!method) assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, count ? "11" : ""));
        if (method == 1) assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == count);
        if (method == 2) assert(out.type == FVM_VALUE_NUMBER && out.number == count + 1);
        if (method >= 2 && (method != 3 || !count)) { assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == (method == 4 ? 4 : method == 3 ? 1 : count + 1)); }
        if (method <= 1 || (method == 3 && count)) { if (kind != 2 && (kind == 0 || version < 7)) { assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT); } }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c); if (source) fvm_context_destroy(source);
    }
    fvm_runtime_destroy(rt);
}
#endif

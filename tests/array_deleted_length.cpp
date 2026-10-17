/* Native Array methods follow own length data after deletion and ordinary recreation. */
#define FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
#include "function_apply_deleted_length.cpp"
static FVM_Object *array_deleted_fixture(FVM_Context *c, unsigned kind, FVM_Value &raw, unsigned &count)
{
    FVM_Object *array = 0; fvm_value_undefined(&raw); count = 0;
    if (!kind) { FVM_Value pair[2]; fvm_value_number(pair, 11); fvm_value_number(pair + 1, 22); assert(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK); fvm_value_number(&raw, 2); count = 2; return array; }
    array = deleted_length_array(c);
    if (kind >= 2 && kind <= 7) {
        if (kind == 2) fvm_value_string(&raw, "1");
        if (kind == 3) fvm_value_number(&raw, 1);
        if (kind == 4) fvm_value_number(&raw, 1.9);
        if (kind == 5) fvm_value_boolean(&raw, 1);
        if (kind == 6) raw.type = FVM_VALUE_NULL;
        count = kind <= 5 ? 1 : 0;
        assert(fvm_object_set(c, array, "length", &raw) == FVM_OK);
    }
    if (kind == 8 || kind == 9) {
        FVM_Object *proto = 0, *old = array->prototype; assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK && fvm_object_set_prototype(c, proto, old) == FVM_OK && fvm_object_set_prototype(c, array, proto) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 2); assert(fvm_object_set(c, proto, "length", &value) == FVM_OK);
        if (kind == 9) { Program p; p.number(77); p.op(0x2a); p.op(0); FVM_Object *fn = 0; assert(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK); FVM_Value args[3], out; fvm_value_string(args, "length"); args[1] = class_value(fn); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL; assert(fvm_object_call(c, proto, "addProperty", 3, args, &out) == FVM_OK && out.boolean); }
    }
    if (kind == 10) { fvm_value_number(&raw, 1); assert(fvm_object_set(c, array, "LeNgTh", &raw) == FVM_OK); count = c->swf_info.version < 7 ? 1 : 0; }
    return array;
}
static FVM_Value array_deleted_call(FVM_Context *c, FVM_Object *array, const char *method, unsigned route)
{
    FVM_Value target = class_value(array), arg, out; assert(fvm_set_global(c, "arrayDeletedTarget", &target) == FVM_OK);
    unsigned argc = 0; fvm_value_number(&arg, 81);
    if (!std::strcmp(method, "push") || !std::strcmp(method, "unshift")) argc = 1;
    if (!route) assert(fvm_object_call(c, array, method, argc, &arg, &out) == FVM_OK);
    else { Program p; if (argc) p.number(81); p.number(argc); p.variable("arrayDeletedTarget"); p.text(method); p.op(0x52); p.finish(); out = boxing_run(c, p, route - 1, FVM_OK); }
    return out;
}
#ifndef FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"join", "slice", "concat", "reverse", "sort", "pop", "shift", "push", "unshift"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind <= 10; ++kind)
    for (unsigned method = 0; method < 9; ++method)
    for (unsigned route = 0; route < 4; ++route)
    for (unsigned transport = 0; transport < 2; ++transport) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0, *source = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value raw, value, out; unsigned count = 0; FVM_Object *array = array_deleted_fixture(c, kind, raw, count);
        if (transport) { source = c; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK); value = class_value(array); unsigned char *wire = 0; size_t bytes = 0; assert(fvm_value_serialize(source, &value, &wire, &bytes) == FVM_OK && fvm_value_deserialize(c, wire, bytes, &value) == FVM_OK); fvm_serialized_free(wire); array = value.object; }
        assert(fvm_gc_pin(c, array) == FVM_OK); out = array_deleted_call(c, array, methods[method], route);
        if (!method) assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, count == 2 ? "11,22" : count == 1 ? "11" : ""));
        else if (method <= 2) { assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == count); }
        else if (method <= 4) assert(out.type == FVM_VALUE_OBJECT && out.object == array);
        else if (method <= 6) { if (!count) assert(out.type == FVM_VALUE_UNDEFINED); else assert(out.type == FVM_VALUE_NUMBER && out.number == (method == 5 && count == 2 ? 22 : 11)); }
        else assert(out.type == FVM_VALUE_NUMBER && out.number == count + 1);
        const bool changed = method >= 7 || (method >= 5 && count);
        if (changed) { assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == (method >= 7 ? count + 1 : count - 1)); }
        else if (kind <= 7 && kind != 1) { assert(fvm_object_get(c, array, "length", &value) == FVM_OK); boxing_equal(value, raw); }
        else if (kind != 10 || version >= 7) { int own = 1; assert(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !own); }
        /* Shrinking from ordinary String/Boolean data must not use stale numeric native tails. */
        if ((kind == 2 || kind == 5) && (method == 5 || method == 6)) assert(fvm_table_get_index(c, array, 1, &value) == FVM_OK && value.number == 22);
        assert(!c->exception_pending && !c->gc_frames && !c->gc_native_depth && !c->script_depth); fvm_context_destroy(c); if (source) fvm_context_destroy(source);
    }
    fvm_runtime_destroy(rt);
}
#endif

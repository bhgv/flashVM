/* Many-field preparation owns converted names while later self-removing callbacks collect. */
#define FVM_MANY_FIELDS_FIXTURE_ONLY
#include "array_sort_on_many_fields.cpp"
#include <cstdio>
static unsigned many_calls, many_fault, many_indices[65];
static FVM_Result many_name(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *data)
{
    unsigned index = *(unsigned *)data; assert(index == many_calls++ && frame->capture);
    char key[32]; std::sprintf(key, "%u", index); int removed = 0;
    assert(fvm_object_delete(c, frame->capture, key, &removed) == FVM_OK && removed);
    if (!index) { assert(fvm_object_delete(c, c->global, "manyFieldFields", &removed) == FVM_OK && removed); assert(fvm_object_delete(c, c->global, "manyFieldReceiver", &removed) == FVM_OK && removed); }
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    if (index == 64 && many_fault) { fvm_value_number(out, 77); return many_fault == 1 ? FVM_ERROR : fvm_exception_raise(c, out); }
    return fvm_value_string(out, index == 64 ? "key" : "same");
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned i = 0; i < 65; ++i) many_indices[i] = i;
    for (unsigned version = 4; version <= 8; ++version)
    for (many_fault = 0; many_fault < 3; ++many_fault)
    for (unsigned route = 0; route < 5; ++route) {
        if ((route == 2 || route == 3) && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = many_fields_receiver(c), *fields = many_fields_list(c, 65, false); FVM_Value value, options; fvm_value_number(&options, 16);
        for (unsigned i = 0; i < 65; ++i) { FVM_Object *name = 0, *fn = version_function(c, 0); assert(fvm_construct(c, "Object", 0, 0, &name) == FVM_OK && fvm_function_set_body(c, fn, many_name, many_indices + i, fields) == FVM_OK); value = class_value(fn); assert(fvm_object_set(c, name, "toString", &value) == FVM_OK); value = class_value(name); assert(fvm_table_set_index(c, fields, i, &value) == FVM_OK); }
        const FVM_Result expected = !many_fault ? FVM_OK : many_fault == 1 ? FVM_ERROR : FVM_EXCEPTION; many_calls = 0;
        if (route == 4) {
            value = class_value(fields); assert(fvm_set_global(c, "manyFieldFields", &value) == FVM_OK); value = class_value(receiver); assert(fvm_set_global(c, "manyFieldReceiver", &value) == FVM_OK);
            FVM_Object *owner = receiver; FVM_NativeEntry entry; bool found = false; for (; owner && !found; owner = owner->prototype) { fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = owner->methods.find("sortOn"); if (method != owner->methods.end()) { entry = method->second; found = true; } }
            assert(found); FVM_Value args[2] = {class_value(fields),options}; assert(entry.callback(c, receiver, 2, args, &value, entry.userdata) == expected); if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
        } else value = many_fields_call(c, receiver, fields, options, route, expected);
        assert(many_calls == 65); if (!many_fault) assert(value.type == FVM_VALUE_OBJECT && value.object == receiver);
        const unsigned original[] = {2,1,3}; for (unsigned i = 0; i < 3; ++i) { FVM_Value item; assert(fvm_table_get_index(c, receiver, i, &item) == FVM_OK && fvm_object_get(c, item.object, "key", &value) == FVM_OK && value.number == (many_fault ? original[i] : i + 1)); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

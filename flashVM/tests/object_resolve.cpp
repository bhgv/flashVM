/* Missing loaded properties call raw __resolve, skipping primitives but stopping at Objects. */
#define main call_case_fixture_main
#include "object_call_case.cpp"
#undef main
#include "flashvm/table.h"
#include "flashvm/execution.h"
static unsigned resolve_ids[] = {1, 2}, resolve_calls, resolve_source, resolve_fault, resolver_getters;
static FVM_Object *resolve_receiver;
static FVM_Result resolve_expected;
static const char *resolve_requested;
static size_t resolve_size;
static FVM_Result resolve_result(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    ++resolve_calls; resolve_source = *(unsigned *)userdata;
    assert(receiver == resolve_receiver && n == 1 && fvm_value_string_size(args) == resolve_size && !std::memcmp(args[0].string, resolve_requested, resolve_size));
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (resolve_fault) { fvm_value_number(out, 71); return resolve_fault == 1 ? fvm_exception_raise(c, out) : resolve_fault == 3 ? FVM_NOT_FOUND : FVM_ERROR; }
    *out = args[0]; return FVM_OK;
}
static FVM_Result resolve_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *userdata)
{ return resolve_result(c, frame->receiver, frame->argc, frame->argv, out, userdata); }
static FVM_Result resolver_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ ++resolver_getters; return FVM_ERROR; }
#ifdef FVM_RESOLVE_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
static FVM_Result resolve_read(FVM_Context *c, FVM_Object *receiver, unsigned query, FVM_Value *out)
{
#ifdef FVM_RESOLVE_JIT
    FVM_Value value = call_object(receiver); assert(fvm_set_global(c, "resolveTarget", &value) == FVM_OK);
    fvm_value_string_bytes(&value, resolve_requested, resolve_size); assert(fvm_set_global(c, "resolveName", &value) == FVM_OK);
    Program code; code.variable("resolveTarget"); code.variable("resolveName"); code.op(0x4e); code.finish();
    *out = class_run(c, code, resolve_expected); return resolve_expected;
#else
    return query == 1 ? fvm_table_get_index(c, receiver, 12, out) : fvm_object_get_bytes(c, receiver, resolve_requested, resolve_size, out);
#endif
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_RESOLVE_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 0; version <= 8; ++version) for (unsigned kind = 0; kind < 7; ++kind) for (unsigned spelling = 0; spelling < 2; ++spelling) for (unsigned query = 0; query < 3; ++query) for (resolve_fault = 0; resolve_fault < 4; ++resolve_fault) {
        if (version && version < 4) continue;
#ifdef FVM_RESOLVE_JIT
        if (!version) continue;
#endif
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *parent = 0, *own = 0, *inherited = 0; const unsigned char end[] = {0}; FVM_Value value, out, args[3];
        assert(fvm_construct(c, "Object", 0, 0, &resolve_receiver) == FVM_OK && fvm_gc_pin(c, resolve_receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, resolve_receiver, parent) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &own) == FVM_OK && fvm_function_set_body(c, own, resolve_body, resolve_ids, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &inherited) == FVM_OK && fvm_function_set_body(c, inherited, resolve_body, resolve_ids + 1, 0) == FVM_OK);
        const char *resolver_name = spelling ? "__ReSoLvE" : "__resolve";
        if (kind == 1) { value = call_object(inherited); assert(fvm_object_set(c, parent, resolver_name, &value) == FVM_OK); }
        if (kind == 2 || kind == 3 || kind == 6) { value = call_object(inherited); assert(fvm_object_set(c, parent, "__resolve", &value) == FVM_OK); }
        if (kind == 0 || kind == 4) { value = call_object(own); assert(fvm_object_set(c, resolve_receiver, resolver_name, &value) == FVM_OK); }
        if (kind == 2) { fvm_value_number(&value, 7); assert(fvm_object_set(c, resolve_receiver, resolver_name, &value) == FVM_OK); }
        if (kind == 3) { FVM_Object *plain = 0; assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK); value = call_object(plain); assert(fvm_object_set(c, resolve_receiver, resolver_name, &value) == FVM_OK); }
        if (kind == 4) {
            FVM_Object *trap = 0; assert(fvm_function_create_native(c, resolver_getter, 0, 0, &trap) == FVM_OK);
            fvm_value_string(args, resolver_name); args[1] = call_object(trap); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
            assert(fvm_object_call(c, resolve_receiver, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        }
        if (kind == 5) { assert(fvm_object_define_method_rooted(c, resolve_receiver, resolver_name, resolve_result, resolve_ids, 0) == FVM_OK); resolve_receiver->methods.find(fvm_detail::string(resolver_name))->second.collector_entry = true; }
        if (kind == 6) assert(fvm_object_define_accessor_rooted(c, resolve_receiver, resolver_name, resolver_getter, 0, 0, 0) == FVM_OK);
        fvm_value_undefined(&value); assert(fvm_object_set(c, resolve_receiver, "present", &value) == FVM_OK);
        resolve_requested = query == 0 ? "MiSs\0Name" : query == 1 ? "12" : "\xc3\x84\0X"; resolve_size = query == 0 ? 9 : query == 1 ? 2 : 4;
        bool active = version && (!spelling || version <= 6);
        unsigned source = kind == 1 ? (active ? 2 : 0) : kind == 2 || kind == 6 ? (version ? 2 : 0) : kind == 3 ? (version && !active ? 2 : 0) : active ? 1 : 0;
        resolve_calls = resolver_getters = resolve_source = 0;
        FVM_Result expected = source && resolve_fault ? (resolve_fault == 1 ? FVM_EXCEPTION : resolve_fault == 3 ? FVM_NOT_FOUND : FVM_ERROR) : FVM_OK;
        resolve_expected = expected;
        FVM_Result status = resolve_read(c, resolve_receiver, query, &out);
        assert(status == expected && resolve_calls == (source ? 1U : 0U) && resolve_source == source && !resolver_getters);
        if (status == FVM_OK && source) {
            assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == resolve_size && !std::memcmp(out.string, resolve_requested, resolve_size));
            assert(fvm_set_global(c, "resolvedCopy", &out) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && fvm_get_global(c, "resolvedCopy", &out) == FVM_OK && fvm_value_string_size(&out) == resolve_size && !std::memcmp(out.string, resolve_requested, resolve_size));
        } else if (status == FVM_OK) assert(out.type == FVM_VALUE_UNDEFINED);
#ifndef FVM_RESOLVE_JIT
        if (status == FVM_EXCEPTION) assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 71 && fvm_exception_clear(c) == FVM_OK);
#endif
        resolve_calls = 0; int present = 1; assert(fvm_object_has_own(c, resolve_receiver, "missing", &present) == FVM_OK && !present && !resolve_calls);
        assert(fvm_object_get(c, resolve_receiver, "present", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && !resolve_calls);
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_owner && fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

/* Resolver-returned Functions execute with original this/args; missing methods differ by C/AVM1 contract. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#include "flashvm/execution.h"
#include "flashvm/callable.h"
#include "flashvm/property.h"
#include "flashvm/exception.h"
#include <cstring>
static FVM_Object *resolved_receiver, *resolved_owner, *resolved_argument;
static unsigned resolved_mode, resolved_calls, resolved_method_calls;
static FVM_Value resolved_args[2];
static FVM_Result resolved_stack_trap(FVM_Context *, FVM_Stack *, void *)
{ assert(false); return FVM_ERROR; }
static FVM_Value resolved_value(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static bool resolved_live(FVM_Context *c, FVM_Object *object)
{ for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == object) return true; return false; }
static FVM_Result resolved_method(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++resolved_method_calls; assert(frame->receiver == resolved_receiver && !c->calling_owner && frame->argc == 2);
    assert(frame->argv[0].object == resolved_argument && fvm_value_string_size(frame->argv + 1) == 3 && !std::memcmp(frame->argv[1].string, "a\0b", 3));
    assert(fvm_gc_collect(c, 0) == FVM_OK && resolved_live(c, resolved_argument));
    FVM_Value marker; assert(fvm_object_get(c, resolved_argument, "marker", &marker) == FVM_OK && marker.number == 55);
    fvm_value_number(out, resolved_mode == 7 ? 77 : 55);
    return resolved_mode == 6 ? FVM_UNSUPPORTED : resolved_mode == 7 ? fvm_exception_raise(c, out) : FVM_OK;
}
static FVM_Result resolving_method(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++resolved_calls; assert(frame->receiver == resolved_receiver && frame->argc == 1 && fvm_value_string_size(frame->argv) == 14 && !std::memcmp(frame->argv[0].string, "Missing\0Method", 14));
    int removed = 0; assert(fvm_object_delete(c, resolved_owner, "__resolve", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, resolved_receiver, 0) == FVM_OK);
    fvm_value_undefined(resolved_args); fvm_value_undefined(resolved_args + 1);
    assert(fvm_gc_collect(c, 0) == FVM_OK && resolved_live(c, resolved_argument));
    if (resolved_mode == 4) { fvm_value_number(out, 74); return fvm_exception_raise(c, out); }
    if (resolved_mode == 5) return FVM_UNSUPPORTED;
    if (resolved_mode == 1) { fvm_value_number(out, 29); return FVM_OK; }
    if (resolved_mode == 8) { FVM_Object *plain = 0; assert(fvm_callable_create(c, resolved_stack_trap, 0, &plain) == FVM_OK); *out = resolved_value(plain); return FVM_OK; }
    if (resolved_mode == 2) { FVM_Object *plain = 0; assert(fvm_object_create(c, &plain) == FVM_OK); *out = resolved_value(plain); return FVM_OK; }
    *out = resolved_value(frame->capture); return FVM_OK;
}
#ifdef FVM_RESOLVE_CALL_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_RESOLVE_CALL_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 0; version <= 8; ++version) for (unsigned parent = 0; parent < 2; ++parent) for (unsigned api = 0; api < 2; ++api) for (resolved_mode = 0; resolved_mode < 9; ++resolved_mode) {
        if ((version && version < 4) || (parent && version == 4)) continue;
#ifdef FVM_RESOLVE_CALL_JIT
        if (!version || !api) continue;
#endif
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *target = 0, *resolver = 0, *method = 0;
        if (parent) {
            FVM_Object *unused = 0; super_accessor_setup(c, &resolved_receiver, &unused, &target);
            FVM_Value base; assert(fvm_get_global(c, "Base", &base) == FVM_OK); resolved_owner = prototype(c, base);
        } else {
            assert(fvm_construct(c, "Object", 0, 0, &resolved_receiver) == FVM_OK && fvm_gc_pin(c, resolved_receiver) == FVM_OK);
            target = resolved_owner = resolved_receiver;
        }
        const unsigned char end[] = {0}; assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &method) == FVM_OK && fvm_function_set_body(c, method, resolved_method, 0, 0) == FVM_OK);
        if (resolved_mode != 3) {
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &resolver) == FVM_OK && fvm_function_set_body(c, resolver, resolving_method, 0, method) == FVM_OK);
            FVM_Value value = resolved_value(resolver); assert(fvm_object_set(c, resolved_owner, "__resolve", &value) == FVM_OK);
        }
        assert(fvm_object_create(c, &resolved_argument) == FVM_OK); FVM_Value value = resolved_value(resolved_argument); assert(fvm_object_set(c, resolved_argument, "self", &value) == FVM_OK);
        fvm_value_number(&value, 55); assert(fvm_object_set(c, resolved_argument, "marker", &value) == FVM_OK); resolved_args[0] = resolved_value(resolved_argument);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_value_to_string(c, &value, resolved_args + 1) == FVM_OK);
        bool active = version && resolved_mode != 3, callable = active && (resolved_mode == 0 || resolved_mode == 6 || resolved_mode == 7);
        FVM_Result expected = active && (resolved_mode == 4 || resolved_mode == 7) ? FVM_EXCEPTION : active && (resolved_mode == 5 || resolved_mode == 6) ? FVM_UNSUPPORTED : callable ? FVM_OK : !api || (!version && parent) ? FVM_UNSUPPORTED : FVM_OK;
        resolved_calls = resolved_method_calls = 0; FVM_Value out, key; fvm_value_string_bytes(&key, "Missing\0Method", 14); FVM_Result status;
#ifdef FVM_RESOLVE_CALL_JIT
        value = resolved_value(target); assert(fvm_set_global(c, "resolvedTarget", &value) == FVM_OK && fvm_set_global(c, "resolvedName", &key) == FVM_OK);
        assert(fvm_set_global(c, "resolvedFirst", resolved_args) == FVM_OK && fvm_set_global(c, "resolvedSecond", resolved_args + 1) == FVM_OK);
        Program call; call.variable("resolvedSecond"); call.variable("resolvedFirst"); call.number(2); call.variable("resolvedTarget"); call.variable("resolvedName"); call.op(0x52); call.finish(); out = class_run(c, call, expected); status = expected;
#else
        status = api ? fvm_exec_invoke_member(c, target, &key, 2, resolved_args, &out) : fvm_object_call_bytes(c, target, key.string, fvm_value_string_size(&key), 2, resolved_args, &out);
#endif
        assert(status == expected && resolved_calls == (active ? 1U : 0U) && resolved_method_calls == (callable ? 1U : 0U));
        if (status == FVM_OK) { if (callable) assert(out.type == FVM_VALUE_NUMBER && out.number == 55); else assert(out.type == FVM_VALUE_UNDEFINED); }
#ifndef FVM_RESOLVE_CALL_JIT
        if (status == FVM_EXCEPTION) assert(fvm_exception_get(c, &out) == FVM_OK && out.number == (resolved_mode == 4 ? 74 : 77) && fvm_exception_clear(c) == FVM_OK);
#endif
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

/* Folded inherited script setters retain stored name, receiver and detached owner through GC. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static FVM_Object *expected_receiver, *expected_owner;
static unsigned setter_calls, setter_fault;
static FVM_Value setter_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static FVM_Result case_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{ fvm_value_number(out, 17); return FVM_OK; }
static FVM_Result case_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++setter_calls;
    assert(frame->receiver == expected_receiver && frame->argc == 1 && frame->argv[0].type == FVM_VALUE_STRING);
    assert(fvm_value_string_size(frame->argv) == 3 && !std::memcmp(frame->argv[0].string, "a\0b", 3));
    assert(c->calling_setter_owner == expected_owner && c->calling_setter_name && *c->calling_setter_name == fvm_detail::string("MiXeD\0Name", 10));
    if (setter_fault == 3) {
        int removed = 0;
        assert(fvm_object_set_prototype(c, expected_receiver, 0) == FVM_OK);
        assert(fvm_object_delete_bytes(c, expected_owner, "MiXeD\0Name", 10, &removed) == FVM_OK && removed);
    }
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    bool live = false; for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == expected_owner) live = true;
    assert(live && *c->calling_setter_name == fvm_detail::string("MiXeD\0Name", 10));
    if (setter_fault == 1) return FVM_ERROR;
    if (setter_fault == 2) { FVM_Value error; fvm_value_string_bytes(&error, "setter\0failed", 13); return fvm_exception_raise(c, &error); }
    fvm_value_undefined(out); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        const bool folded = version && version <= 6;
        for (setter_fault = 0; setter_fault < 4; ++setter_fault) {
            FVM_Object *receiver = 0, *parent = 0, *get = 0, *set = 0; FVM_Value args[3], out, value;
            assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
            const unsigned char end[] = {0};
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, case_get, 0, 0) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, case_set, 0, 0) == FVM_OK);
            fvm_value_string_bytes(args, "MiXeD\0Name", 10); args[1] = setter_object(get); args[2] = setter_object(set);
            assert(fvm_object_call(c, parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            expected_receiver = receiver; expected_owner = parent; setter_calls = 0;
            fvm_value_string_bytes(&value, "a\0b", 3);
            FVM_Result status = fvm_object_set_bytes(c, receiver, "mixed\0name", 10, &value);
            assert(setter_calls == (folded ? 1U : 0U));
            if (folded && (setter_fault == 1 || setter_fault == 2)) {
                assert(status == (setter_fault == 1 ? FVM_ERROR : FVM_EXCEPTION));
                if (setter_fault == 2) { assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 13 && !std::memcmp(out.string, "setter\0failed", 13)); assert(fvm_exception_clear(c) == FVM_OK); }
            } else assert(status == FVM_OK);
            int own = 0; assert(fvm_object_has_own_bytes(c, receiver, "mixed\0name", 10, &own) == FVM_OK && !!own == !folded);
            assert(!c->calling_setter_owner && !c->calling_setter_name && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

/* addProperty replaces a folded own name, retaining flags/data without invoking old getters. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/base_state.hpp"
#include <cassert>
static unsigned definition_gets;
static FVM_Result definition_get(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{ ++definition_gets; assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK); fvm_value_number(out, 17); return FVM_OK; }
static FVM_Value definition_object(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *getter = 0; const unsigned char end[] = {0}; FVM_Value value, args[3], out;
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK && fvm_function_set_body(c, getter, definition_get, 0, 0) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK);
        assert(fvm_object_set_property_flags_bytes(c, object, "MiXeD\0Name", 10, 3, 0) == FVM_OK);
        fvm_value_string_bytes(args, "mixed\0name", 10); args[1] = definition_object(getter); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
        definition_gets = 0;
        assert(fvm_object_call(c, object, "addProperty", 3, args, &out) == FVM_OK && out.boolean && !definition_gets);
        const bool folded = version && version <= 6;
        unsigned flags = 0;
        assert(fvm_object_property_flags_bytes(c, object, "mixed\0name", 10, &flags) == FVM_OK && flags == (folded ? 3U : 4U));
        fvm_detail::string selected(folded ? "MiXeD\0Name" : "mixed\0name", 10);
        assert(object->accessors.find(selected) != object->accessors.end());
        assert(fvm_base_accessor_data(object->accessors.find(selected)->second, value));
        if (folded) assert(value.type == FVM_VALUE_NUMBER && value.number == 9); else assert(value.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get_bytes(c, object, "mixed\0name", 10, &out) == FVM_OK && out.number == 17 && definition_gets == 1);
        if (folded) assert(object->properties.find(fvm_detail::string("MiXeD\0Name", 10)) == object->properties.end());
        else assert(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &out) == FVM_OK && out.number == 9);
        fvm_value_number(&value, 23); assert(fvm_object_set_bytes(c, object, "mixed\0name", 10, &value) == FVM_OK);
        assert(fvm_base_accessor_data(object->accessors.find(selected)->second, value));
        if (folded) assert(value.type == FVM_VALUE_NUMBER && value.number == 23); else assert(value.type == FVM_VALUE_UNDEFINED);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

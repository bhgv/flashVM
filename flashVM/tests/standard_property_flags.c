/* ASSetPropFlags gives set bits priority and touches only existing own properties. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0, *proto = 0;
    FVM_Value args[4], value, out;
    unsigned int initial, set, clear, flags, version;
    int own = 0;
    const char nul[] = {'n', 0, 'm'};
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (version = 0; version <= 8; ++version) {
    unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    if (version && version < 4) continue;
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_object_create(c, &o) == FVM_OK);
    assert(fvm_gc_pin(c, o) == FVM_OK);
    fvm_value_number(&value, 9);
    assert(fvm_object_set(c, o, "", &value) == FVM_OK);
    assert(fvm_object_set(c, o, "undefined", &value) == FVM_OK);
    if (version) {
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    }
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(&value, 9);
    assert(fvm_object_set(c, o, "x", &value) == FVM_OK);
    fvm_value_undefined(args); args[0].type = FVM_VALUE_OBJECT; args[0].object = o;
    fvm_value_string(args + 1, "x");
    for (initial = 0; initial < 8; ++initial)
        for (set = 0; set < 8; ++set)
            for (clear = 0; clear < 8; ++clear) {
                assert(fvm_object_set_property_flags(c, o, "x", 0, 7) == FVM_OK);
                assert(fvm_object_set_property_flags(c, o, "x", initial, 0) == FVM_OK);
                fvm_value_number(args + 2, set); fvm_value_number(args + 3, clear);
                assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
                assert(fvm_object_property_flags(c, o, "x", &flags) == FVM_OK && flags == ((initial & ~clear) | set));
            }
    assert(fvm_object_set_bytes(c, o, nul, sizeof(nul), &value) == FVM_OK);
    fvm_value_string_bytes(args + 1, nul, sizeof(nul)); fvm_value_number(args + 2, 4); fvm_value_number(args + 3, 4);
    assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
    assert(fvm_object_property_flags_bytes(c, o, nul, sizeof(nul), &flags) == FVM_OK && flags == 4);
    fvm_value_number(&value, 81); assert(fvm_object_set_bytes(c, o, nul, sizeof(nul), &value) == FVM_OK);
    assert(fvm_object_get_bytes(c, o, nul, sizeof(nul), &value) == FVM_OK && value.number == 9);
    assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
    assert(fvm_object_set(c, proto, "inherited", &value) == FVM_OK); assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
    fvm_value_string(args + 1, "missing,inherited");
    assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
    assert(fvm_object_has_own(c, o, "missing", &own) == FVM_OK && !own);
    assert(fvm_object_property_flags(c, proto, "inherited", &flags) == FVM_OK && flags == 0);
    fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL; fvm_value_number(args + 2, 2); fvm_value_number(args + 3, 2);
    assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
    assert(fvm_object_property_flags(c, o, "x", &flags) == FVM_OK && (flags & 2));
    assert(fvm_object_property_flags_bytes(c, o, nul, sizeof(nul), &flags) == FVM_OK && (flags & 2));
    assert(fvm_object_set_property_flags(c, o, "x", 0, 7) == FVM_OK);
    assert(fvm_object_set_property_flags(c, o, "", 0, 7) == FVM_OK);
    assert(fvm_object_set_property_flags(c, o, "undefined", 0, 7) == FVM_OK);
    fvm_value_undefined(args + 1); fvm_value_number(args + 2, 4); fvm_value_number(args + 3, 0);
    assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
    assert(fvm_object_property_flags(c, o, "", &flags) == FVM_OK && flags == (version && version < 7 ? 4U : 0U));
    assert(fvm_object_property_flags(c, o, "undefined", &flags) == FVM_OK && flags == (version && version < 7 ? 0U : 4U));
    assert(fvm_object_property_flags(c, o, "x", &flags) == FVM_OK && flags == 0);
    assert(fvm_call(c, "ASSetPropFlags", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_call(c, "ASSetPropFlags", 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    args[1].type = FVM_VALUE_NULL;
    assert(fvm_call(c, "ASSetPropFlags", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_property_flags(c, o, "x", &flags) == FVM_OK && flags == 0);
    fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

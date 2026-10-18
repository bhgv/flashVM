/* Loaded call/apply use depth one for super independently of function storage and chain length. */
#include "property_version_fixture.hpp"
#include "../src/avm1/encoding_constants.h"
static FVM_Object *adapter_super_function(FVM_Context *c, unsigned kind)
{
    Program body;
    if (kind == 1 || kind == 3) { body.op(0x96); body.op(2); body.op(0); body.op(4); body.op(1); }
    else body.variable("super");
    body.finish(); FVM_Object *function = 0;
    if (!kind) { assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK); return function; }
    fvm_detail::vector<unsigned char> header, code;
    const char *name = "adapterFunction2"; for (size_t i = 0; i <= std::strlen(name); ++i) header.push_back((unsigned char)name[i]);
    header.push_back(0); header.push_back(0); header.push_back(2);
    unsigned flags = kind == 1 ? FVM_AVM1_F2_PRELOAD_SUPER : kind == 2 ? FVM_AVM1_F2_SUPPRESS_SUPER : kind == 3 ? FVM_AVM1_F2_PRELOAD_SUPER | FVM_AVM1_F2_SUPPRESS_SUPER : 0;
    header.push_back((unsigned char)flags); header.push_back((unsigned char)(flags >> 8));
    header.push_back((unsigned char)body.bytes.size()); header.push_back((unsigned char)(body.bytes.size() >> 8));
    code.push_back(0x8e); code.push_back((unsigned char)header.size()); code.push_back((unsigned char)(header.size() >> 8));
    for (size_t i = 0; i < header.size(); ++i) code.push_back(header[i]);
    for (size_t i = 0; i < body.bytes.size(); ++i) code.push_back(body.bytes[i]);
    code.push_back(0);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK); FVM_Value out;
    assert(fvm_get_global(c, name, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); return out.object;
}
#ifndef FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned chain = 0; chain < 4; ++chain)
    for (unsigned location = 0; location < 5; ++location)
    for (unsigned apply = 0; apply < 2; ++apply)
    for (unsigned route = 0; route < 5; ++route) {
        if ((kind && version < 7) || (route >= 3 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[4]; FVM_Value value, args[1], out;
        for (unsigned i = 0; i < 4; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK);
        FVM_Object *function = adapter_super_function(c, kind); value = class_value(function);
        if (location) assert(fvm_object_set(c, nodes[location - 1], "storedFunction", &value) == FVM_OK);
        assert(fvm_set_global(c, "adapterTarget", &value) == FVM_OK); args[0] = class_value(nodes[0]); assert(fvm_set_global(c, "adapterThis", args) == FVM_OK);
        const char *method = apply ? "apply" : "call";
        if (!route) assert(fvm_object_call(c, function, method, 1, args, &out) == FVM_OK);
        else if (route == 1) { FVM_Value adapter; assert(fvm_object_get(c, function, method, &adapter) == FVM_OK && adapter.type == FVM_VALUE_OBJECT); assert(fvm_function_invoke(c, adapter.object, function, 1, args, &out) == FVM_OK); }
        else { Program code; code.variable("adapterThis"); code.number(1); code.variable("adapterTarget"); code.text(method); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 2, FVM_OK); }
        if (kind == 2 || kind == 3) assert(out.type == FVM_VALUE_UNDEFINED);
        else {
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER);
            assert(out.object->resource_result.object->properties["depth"].number == 1);
            FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, facade, "marker", &out) == FVM_OK);
            if (chain >= 2) assert(out.type == FVM_VALUE_NUMBER && out.number == 2); else assert(out.type == FVM_VALUE_UNDEFINED);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner); fvm_context_destroy(c);
    }
    /* Host adapters keep their prior owner-derived facade rather than fixed depth one. */
    for (unsigned apply = 0; apply < 2; ++apply) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *parent = 0; FVM_Value value, out;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        fvm_value_number(&value, 1); assert(fvm_object_set(c, parent, "marker", &value) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
        FVM_Object *function = adapter_super_function(c, 0); value = class_value(function); assert(fvm_object_set(c, receiver, "storedFunction", &value) == FVM_OK);
        value = class_value(receiver); assert(fvm_object_call(c, function, apply ? "apply" : "call", 1, &value, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
        assert(out.object->resource_result.object->properties["depth"].number == 0);
        assert(fvm_object_get(c, out.object, "marker", &out) == FVM_OK && out.number == 1);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

#endif

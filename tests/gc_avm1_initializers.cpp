/* Literal result objects survive actual script GC during key conversion. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK); assert(fvm_extensions_install(c) == FVM_OK);
    FVM_Object *key = 0; assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK);
    object(c, "key", key);
    FVM_Value zero; fvm_value_number(&zero, 0); assert(fvm_set_global(c, "sweeps", &zero) == FVM_OK);
    Bytes body; text(body, "sweeps"); global(body, "sweeps"); body.push_back(0x50); body.push_back(0x1d);
    text(body, "collected"); integer(body, 0); global(body, "gc"); text(body, "collect"); body.push_back(0x52); body.push_back(0x1d);
    text(body, "key_"); text(body, "suffix"); body.push_back(0x21); body.push_back(0x3e); body.push_back(0);
    FVM_Object *fn = 0; assert(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK);
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = fn;
    assert(fvm_object_set(c, key, "toString", &value) == FVM_OK);
    assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
    for (unsigned i = 0; i < 20; ++i) {
        FVM_Object *garbage = 0; assert(fvm_object_create(c, &garbage) == FVM_OK);
        Bytes code; global(code, "key"); text(code, "later"); text(code, "earlier"); text(code, "kept");
        integer(code, 2); code.push_back(0x43); FVM_Value result = run(c, code), property;
        assert(result.type == FVM_VALUE_OBJECT);
        assert(fvm_object_get(c, result.object, "key_suffix", &property) == FVM_OK);
        assert(property.type == FVM_VALUE_STRING && !std::strcmp(property.string, "later"));
        assert(fvm_object_get(c, result.object, "earlier", &property) == FVM_OK);
        assert(property.type == FVM_VALUE_STRING && !std::strcmp(property.string, "kept"));
        assert(fvm_get_global(c, "collected", &property) == FVM_OK && property.type == FVM_VALUE_NUMBER && property.number >= 1);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
    }
    assert(fvm_get_global(c, "sweeps", &value) == FVM_OK && value.number == 20);
    for (unsigned version = 6; version <= 7; ++version) {
        swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        Bytes code; code.push_back(0x96); code.push_back(1); code.push_back(0); code.push_back(3);
        text(code, "value"); integer(code, 1); code.push_back(0x43); FVM_Value result = run(c, code), property;
        assert(fvm_object_get(c, result.object, version < 7 ? "" : "undefined", &property) == FVM_OK);
        assert(property.type == FVM_VALUE_STRING && !std::strcmp(property.string, "value"));
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}

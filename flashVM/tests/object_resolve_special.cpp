/* Special conversions skip __resolve; recursion is bounded for native resolvers too. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
static unsigned special_resolves, recursive_resolves, meta_resolves;
static FVM_Result meta_resolver(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ ++meta_resolves; assert(n == 3); fvm_value_number(out, 91); return FVM_OK; }
static FVM_Result special_resolver(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{ ++special_resolves; assert(n == 1); fvm_value_number(out, 19); return FVM_OK; }
static FVM_Result recursive_resolver(FVM_Context *c, FVM_Object *receiver, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++recursive_resolves; return fvm_object_get(c, receiver, "loop", out); }
static FVM_Value special_object(FVM_Object *o)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = o; return value; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *bare = 0, *normal = 0, *recursive = 0; FVM_Value value, out, zero;
        assert(fvm_object_create(c, &bare) == FVM_OK && fvm_gc_pin(c, bare) == FVM_OK && fvm_object_set_prototype(c, bare, 0) == FVM_OK);
        assert(fvm_function_create_native(c, special_resolver, 0, 0, &normal) == FVM_OK && fvm_gc_pin(c, normal) == FVM_OK);
        value = special_object(normal); assert(fvm_object_set(c, bare, "__resolve", &value) == FVM_OK);
        value = special_object(bare); special_resolves = 0;
        assert(fvm_exec_to_string(c, &value, &out) == FVM_OK && !std::strcmp(out.string, "[type Object]") && !special_resolves);
        assert(fvm_exec_to_number(c, 0, &value, &out) == FVM_OK && !special_resolves); assert(version < 7 ? out.number == 0 : out.number != out.number);
        fvm_value_number(&zero, 0); assert(fvm_exec_compare(c, FVM_EXEC_ADD_DYNAMIC, &value, &zero, &out) == FVM_OK && !special_resolves); assert(version < 7 ? out.number == 0 : out.number != out.number);
        assert(fvm_object_get(c, bare, "ordinary", &out) == FVM_OK && out.number == 19 && special_resolves == 1);
        assert(fvm_function_create_native(c, recursive_resolver, 0, 0, &recursive) == FVM_OK);
        value = special_object(recursive); assert(fvm_object_set(c, bare, "__resolve", &value) == FVM_OK);
        recursive_resolves = 0; assert(fvm_object_get(c, bare, "loop", &out) == FVM_ERROR && recursive_resolves && recursive_resolves <= 128);
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_owner);
        value = special_object(normal); assert(fvm_object_set(c, bare, "__resolve", &value) == FVM_OK);
        bare->prototype = bare; special_resolves = 0; assert(fvm_object_get(c, bare, "cycle", &out) == FVM_ERROR && !special_resolves);
        bare->prototype = 0; assert(fvm_object_get(c, bare, "recovered", &out) == FVM_OK && out.number == 19 && special_resolves == 1);
        FVM_Object *meta = 0, *index = 0, *plain = 0;
        assert(fvm_object_create(c, &meta) == FVM_OK && fvm_function_create_native(c, meta_resolver, 0, 0, &index) == FVM_OK);
        value = special_object(index); assert(fvm_object_set(c, meta, "__index", &value) == FVM_OK && fvm_table_set_metatable(c, bare, meta) == FVM_OK);
        special_resolves = meta_resolves = 0;
        assert(fvm_object_get(c, bare, "meta", &out) == FVM_OK && out.number == 19 && special_resolves == 1 && !meta_resolves);
        assert(fvm_object_create(c, &plain) == FVM_OK); value = special_object(plain); assert(fvm_object_set(c, bare, "__resolve", &value) == FVM_OK);
        assert(fvm_object_get(c, bare, "meta", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && !meta_resolves);
        fvm_value_number(&value, 7); assert(fvm_object_set(c, bare, "__resolve", &value) == FVM_OK);
        assert(fvm_object_get(c, bare, "meta", &out) == FVM_OK && out.number == 91 && meta_resolves == 1);
        int removed = 0; assert(fvm_object_delete(c, bare, "__resolve", &removed) == FVM_OK && removed);
        assert(fvm_object_get(c, bare, "meta", &out) == FVM_OK && out.number == 91 && meta_resolves == 2);
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

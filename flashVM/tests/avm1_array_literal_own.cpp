/* Array literals and split define own slots despite inherited accessors and readonly indices. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
static unsigned calls;
static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; fvm_value_number(out, 99); return FVM_OK; }
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; fvm_value_undefined(out); return FVM_OK; }
static void slots(FVM_Context *c, FVM_Object *o, const char *first, const char *second)
{
    FVM_Value v; assert(fvm_object_get(c, o, "length", &v) == FVM_OK && v.number == 2);
    const char *values[] = {first, second};
    for (unsigned i = 0; i < 2; ++i) {
        assert(fvm_table_get_index(c, o, i, &v) == FVM_OK && v.type == FVM_VALUE_STRING && !std::strcmp(v.string, values[i]));
        int has = 0; assert(fvm_object_has_own_bytes(c, o, i ? "1" : "0", 1, &has) == FVM_OK && has);
    }
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    const unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    FVM_Value klass, proto, v;
    assert(fvm_get_global(c, "Array", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, proto.object, "0", getter, setter, 0, 0) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, proto.object, "1", getter, 0, 0, 0) == FVM_OK);
    assert(fvm_object_set_property_flags(c, proto.object, "1", 4, 0) == FVM_OK);
    Bytes code; text(code, "second"); text(code, "first"); integer(code, 2); code.push_back(0x42);
    FVM_Value result = run(c, code); slots(c, result.object, "first", "second"); assert(calls == 0);
    assert(fvm_gc_collect(c, 0) == FVM_OK); slots(c, result.object, "first", "second");
    fvm_value_string(&v, "first,second"); FVM_Object *string = 0; assert(fvm_construct(c, "String", 1, &v, &string) == FVM_OK);
    fvm_value_string(&v, ","); assert(fvm_object_call(c, string, "split", 1, &v, &result) == FVM_OK);
    slots(c, result.object, "first", "second"); assert(calls == 0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}

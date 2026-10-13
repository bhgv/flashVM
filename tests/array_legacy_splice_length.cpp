/* Loaded splice captures own signed length; negative mutation indices stay explicitly unsupported. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static unsigned converted;
static FVM_Result count(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++converted; fvm_value_number(out, 1); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *o = 0, *proto = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, o, "length", getter, 0, 0, 0) == FVM_OK);
        fvm_value_number(args, 0); fvm_value_number(args + 1, 1); reads = 0;
        assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK && reads == 0); number(c, out.object, "length", 0);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        fvm_value_number(&value, 4); assert(fvm_object_set(c, proto, "length", &value) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
        fvm_value_string(&value, "A"); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
        assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK); number(c, out.object, "length", 0); number(c, o, "length", 0);
        assert(fvm_object_get(c, o, "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "A"));
        fvm_value_number(&value, 4294967299.0); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_string(&value, "C"); assert(fvm_object_set(c, o, "2", &value) == FVM_OK);
        fvm_value_number(args, 1); assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK);
        number(c, out.object, "length", 1); number(c, o, "length", 2);
        assert(fvm_object_get(c, o, "1", &value) == FVM_OK && value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "C"));
        fvm_value_number(&value, 4294967295.0); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        FVM_Object *counter = 0; assert(fvm_construct(c, "Object", 0, 0, &counter) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, counter, "valueOf", count, 0, 0) == FVM_OK);
        args[1] = object(counter); converted = 0;
        assert(borrow(c, array, "splice", o, 2, args, &out) == FVM_UNSUPPORTED && converted == 0);
        number(c, o, "length", 4294967295.0); assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

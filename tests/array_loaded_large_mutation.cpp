/* Loaded generic mutations cross the old 65536 boundary and remain budgeted. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"shift", "unshift", "reverse", "splice"};
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        for (unsigned method = 0; method < 4; ++method) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            if (version) {
                unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
                assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
            }
            assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *array = 0, *source = 0; FVM_Value value, args[2], out;
            assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
            assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK);
            fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            fvm_value_number(&value, 11); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
            fvm_value_number(&value, 22); assert(fvm_object_set(c, source, "65536", &value) == FVM_OK);
            fvm_value_number(args, method == 3 ? 0 : 99); fvm_value_number(args + 1, 1);
            unsigned argc = method == 3 ? 2 : method == 1 ? 1 : 0;
            unsigned long old_limit = rt->config.instruction_limit; rt->config.instruction_limit = 8;
            FVM_Result status = borrow(c, array, methods[method], source, argc, args, &out);
            rt->config.instruction_limit = old_limit;
            assert(status == (version ? FVM_ERROR : FVM_UNSUPPORTED));
            number(c, source, "length", 65537);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            fvm_context_destroy(c);
        }
    }
    for (unsigned variant = 0; variant < 8; ++variant) {
        unsigned method = variant % 4; bool native = variant >= 4;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *source = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, native ? "Array" : "Object", 0, 0, &source) == FVM_OK); assert(fvm_gc_pin(c, source) == FVM_OK);
        if (native) {
            FVM_Object *proto = 0; assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
            assert(fvm_object_define_accessor_rooted(c, proto, "42", getter, 0, 0, 0) == FVM_OK);
            assert(fvm_object_set_prototype(c, source, proto) == FVM_OK);
        }
        reads = 0;
        fvm_value_number(&value, 65537); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        fvm_value_number(&value, 11); assert(fvm_object_set(c, source, "0", &value) == FVM_OK);
        fvm_value_number(&value, 22); assert(fvm_object_set(c, source, "65536", &value) == FVM_OK);
        fvm_value_number(&value, 33); assert(fvm_object_set(c, source, "32768", &value) == FVM_OK);
        fvm_value_number(args, method == 3 ? 65536 : 99); fvm_value_number(args + 1, 1);
        unsigned argc = method == 3 ? 2 : method == 1 ? 1 : 0;
        assert(borrow(c, array, methods[method], source, argc, args, &out) == FVM_OK);
        if (method == 0) { assert(out.type == FVM_VALUE_NUMBER && out.number == 11); number(c, source, "65535", 22);
            if (!native) number(c, source, "65536", 22);
            else { int own = 1; assert(fvm_object_has_own(c, source, "65536", &own) == FVM_OK && !own); } }
        if (method == 1) { assert(out.type == FVM_VALUE_NUMBER && out.number == 65538); number(c, source, "0", 99); number(c, source, "65537", 22); }
        if (method == 2) { assert(out.type == FVM_VALUE_OBJECT && out.object == source); number(c, source, "0", 22); number(c, source, "65536", 11); number(c, source, "32768", 33); }
        if (method == 3) { number(c, out.object, "length", 1); number(c, out.object, "0", 22);
            if (!native) number(c, source, "65536", 22);
            else { int own = 1; assert(fvm_object_has_own(c, source, "65536", &own) == FVM_OK && !own); } }
        number(c, source, "length", method == 3 || (native && method == 0) ? 65536 : native && method == 1 ? 65538 : 65537);
        assert(reads == 0);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

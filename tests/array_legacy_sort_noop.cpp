#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); cfg.instruction_limit = 1; assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *methods = 0, *empty = 0, *wide = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &methods) == FVM_OK); assert(fvm_construct(c, "Array", 0, 0, &empty) == FVM_OK);
        fvm_value_number(&value, 2147483648.0); assert(fvm_construct(c, "Array", 1, &value, &wide) == FVM_OK);
        for (unsigned native = 0; native < 2; ++native) {
            FVM_Object *receiver = 0;
            if (native) { fvm_value_number(&value, 100000); assert(fvm_construct(c, "Array", 1, &value, &receiver) == FVM_OK); }
            else { assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK); assert(fvm_object_define_accessor_rooted(c, receiver, "length", getter, 0, 0, 0) == FVM_OK); }
            fvm_value_number(&value, 81); assert(fvm_table_set_index(c, receiver, 0, &value) == FVM_OK);
            for (unsigned mode = 0; mode < 4; ++mode) {
                reads = 0;
                if (!mode) { fvm_value_string(args, "16"); assert(borrow(c, methods, "sort", receiver, 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED); }
                else if (mode == 1) assert(borrow(c, methods, "sortOn", receiver, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
                else { args[0] = object(mode == 2 ? empty : wide); assert(borrow(c, methods, "sortOn", receiver, 1, args, &out) == FVM_OK && out.object == receiver); }
                assert(!reads); assert(fvm_table_get_index(c, receiver, 0, &value) == FVM_OK && value.number == 81);
                int own = 0; assert(fvm_object_has_own(c, receiver, "1", &own) == FVM_OK && !own);
                assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

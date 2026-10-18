#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static unsigned calls[3], order;
static FVM_Result operand(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{ unsigned i = *(unsigned *)data; ++calls[i]; order = order * 10 + i + 1; fvm_value_number(out, i + 2); return FVM_OK; }
static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; unsigned ids[] = {0,1,2}; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK); FVM_Value math, args[3], out, value; FVM_Object *objects[3], *fn = 0;
        assert(fvm_get_global(c, "Math", &math) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) { assert(fvm_construct(c, "Object", 0, 0, &objects[i]) == FVM_OK); assert(fvm_function_create_native(c, operand, &ids[i], 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, objects[i], "valueOf", &value) == FVM_OK); args[i] = object(objects[i]); }
        const char *methods[] = {"abs", "random", "max", "min", "pow", "atan2"};
        for (unsigned i = 0; i < 6; ++i) {
            calls[0] = calls[1] = calls[2] = order = 0; assert(fvm_object_call(c, math.object, methods[i], 3, args, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER);
            if (version) assert(calls[0] == 1 && calls[1] == 1 && !calls[2] && order == 12);
            if (i == 2) assert(out.number == (version ? 3 : 4)); if (i == 3) assert(out.number == 2);
        }
        assert(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, objects[1], "valueOf", &value) == FVM_OK);
        assert(fvm_object_call(c, math.object, "abs", 2, args, &out) == (version ? FVM_ERROR : FVM_OK)); unsigned long random_before = c->random_state; assert(fvm_object_call(c, math.object, "random", 2, args, &out) == (version ? FVM_ERROR : FVM_OK)); if (version) assert(c->random_state == random_before);
        fvm_value_string(args, "010"); assert(fvm_object_call(c, math.object, "abs", 1, args, &out) == FVM_OK && out.number == (version >= 6 ? 8 : 10));
        assert(fvm_object_call(c, math.object, "abs", 0, 0, &out) == FVM_OK && out.number != out.number);
        fvm_value_number(args, 2); assert(fvm_object_call(c, math.object, "pow", 1, args, &out) == FVM_OK && out.number != out.number);
        fvm_value_number(args, 1); assert(fvm_object_call(c, math.object, "pow", 1, args, &out) == FVM_OK && out.number == 1);
        assert(fvm_object_call(c, math.object, "max", 1, args, &out) == FVM_OK); assert(version ? out.number != out.number : out.number == 1);
        assert(fvm_object_call(c, math.object, "min", 0, 0, &out) == FVM_OK && out.number > 1.0e300);
        assert(fvm_object_call(c, math.object, "max", 0, 0, &out) == FVM_OK && out.number < -1.0e300);
        fvm_value_undefined(args); assert(fvm_object_call(c, math.object, "abs", 1, args, &out) == FVM_OK); assert(version && version <= 6 ? out.number == 0 : out.number != out.number);
        assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

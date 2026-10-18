/* Loaded AVM1 script constructors ignore return values and preserve the newly allocated receiver. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
static FVM_Value constructor_execute(FVM_Context *c, const Program &program, unsigned mode)
{
    FVM_Object *function = 0; FVM_JitCode *code = 0; FVM_Value out;
    assert(fvm_function_create_avm1(c, program.bytes.begin(), program.bytes.size(), 0, 0, 0, &function) == FVM_OK && fvm_gc_pin(c, function) == FVM_OK);
    assert(fvm_jit_compile(program.bytes.begin(), program.bytes.size(), &code) == FVM_OK);
    assert(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    if (mode == 1) assert(fvm_function_set_body(c, function, fvm_jit_body, code, 0) == FVM_OK);
    unsigned long before = c->execution_mode.stats.native_entries;
    assert(fvm_function_invoke(c, function, c->global, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && fvm_gc_pin(c, out.object) == FVM_OK);
    assert(mode ? c->execution_mode.stats.native_entries > before : c->execution_mode.stats.native_entries == before);
    assert(!code->active && !c->gc_frames && !c->gc_native_depth && !c->script_depth);
    assert(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK && fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK);
    return out;
}
static void constructor_check(FVM_Context *c, FVM_Object *instance, const FVM_Value &replacement, unsigned kind, unsigned version)
{
    if (!version && kind >= 2 && kind < 5) { assert(instance == replacement.object); return; }
    assert(instance && (kind < 2 || kind == 5 || instance != replacement.object));
    FVM_Value marker; assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.type == FVM_VALUE_NUMBER && marker.number == 42);
    assert(fvm_gc_pin(c, instance) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 42);
}
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned kind = 0; kind < 6; ++kind) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value replacement; fvm_value_undefined(&replacement);
        if (kind == 1) fvm_value_number(&replacement, 7);
        if (kind == 2) { FVM_Object *object = 0; assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK); replacement = class_value(object); }
        if (kind == 3) { FVM_Object *object = 0; const unsigned char end[] = {0}; assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &object) == FVM_OK); replacement = class_value(object); }
        if (kind == 4) { FVM_Object *object = 0; assert(fvm_construct(c, "Array", 0, 0, &object) == FVM_OK); replacement = class_value(object); }
        assert(fvm_set_global(c, "returnValue", &replacement) == FVM_OK);
        Program body; body.variable("this"); body.text("marker"); body.number(42); body.op(0x4f); body.variable(kind == 5 ? "this" : "returnValue"); body.finish();
        FVM_Object *constructor = 0; assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &constructor) == FVM_OK);
        FVM_Value value = class_value(constructor); assert(fvm_set_global(c, "ReturnCtor", &value) == FVM_OK);
        FVM_Object *holder = 0; assert(fvm_construct(c, "Object", 0, 0, &holder) == FVM_OK && fvm_object_set(c, holder, "ctor", &value) == FVM_OK); value = class_value(holder); assert(fvm_set_global(c, "holder", &value) == FVM_OK);
        FVM_Object *instance = 0; assert(fvm_function_construct(c, constructor, 0, 0, &instance) == FVM_OK); constructor_check(c, instance, replacement, kind, version);
        assert(fvm_construct(c, "ReturnCtor", 0, 0, &instance) == FVM_OK); constructor_check(c, instance, replacement, kind, version);
        if (fvm_jit_available()) for (unsigned mode = 0; mode < 3; ++mode) for (unsigned method = 0; method < 2; ++method) {
            Program code; code.number(0); if (method) { code.variable("holder"); code.text("ctor"); } else code.text("ReturnCtor"); code.op(method ? 0x53 : 0x40); code.finish();
            value = constructor_execute(c, code, mode); constructor_check(c, value.object, replacement, kind, version);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime); return 0;
}

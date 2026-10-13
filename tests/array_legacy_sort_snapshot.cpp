/* Loaded sort captures all own values before coercion and materializes native holes. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static FVM_Object *target, *rows[3];
static unsigned calls;
static bool fail;
static FVM_Result text(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    bool rooted = false;
    for (FVM_GCRootFrame *frame = c->gc_frames; frame; frame = frame->previous)
        if (frame->stack && frame->stack->size() == 3 &&
            (*frame->stack)[0].object == rows[0] && (*frame->stack)[1].object == rows[1] &&
            (*frame->stack)[2].object == rows[2]) rooted = true;
    assert(rooted);
    if (!calls++) {
        FVM_Value length; fvm_value_number(&length, 0);
        assert(fvm_object_set(c, target, "length", &length) == FVM_OK);
        if (fail) return FVM_ERROR;
    }
    return fvm_value_string(out, self == rows[0] ? "2" : self == rows[1] ? "1" : "3");
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[3], out, value; FVM_Object *fn = 0;
        assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); assert(fvm_gc_pin(c, fn) == FVM_OK);
        for (unsigned mode = 0; mode < 2; ++mode) {
            for (unsigned i = 0; i < 3; ++i) {
                assert(fvm_construct(c, "Object", 0, 0, rows + i) == FVM_OK);
                value = object(fn); assert(fvm_object_set(c, rows[i], "toString", &value) == FVM_OK);
                values[i] = object(rows[i]);
            }
            assert(fvm_construct(c, "Array", 3, values, &target) == FVM_OK); assert(fvm_gc_pin(c, target) == FVM_OK);
            calls = 0; fail = mode != 0;
            assert(fvm_object_call(c, target, "sort", 0, 0, &out) == (fail ? FVM_ERROR : FVM_OK));
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
            if (!fail) {
                assert(calls == 6); assert(fvm_gc_collect(c, 0) == FVM_OK);
                assert(fvm_table_get_index(c, target, 0, &value) == FVM_OK && value.object == rows[1]);
                assert(fvm_table_get_index(c, target, 1, &value) == FVM_OK && value.object == rows[0]);
                assert(fvm_table_get_index(c, target, 2, &value) == FVM_OK && value.object == rows[2]);
            } else number(c, target, "length", 0);
        }
        fvm_value_number(&value, 3); assert(fvm_construct(c, "Array", 1, &value, &target) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_table_set_index(c, target, 2, &value) == FVM_OK);
        assert(fvm_object_call(c, target, "sort", 0, 0, &out) == FVM_OK);
        assert(fvm_table_get_index(c, target, version < 7 ? 0 : 2, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
        for (unsigned i = 0; i < 3; ++i) { char key[2] = {(char)('0' + i), 0}; int own = 0; assert(fvm_object_has_own(c, target, key, &own) == FVM_OK && own); }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

/* Failed SWF loads publish neither Function adapter nor Error method flag changes. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
static void activation_flags(FVM_Context *c, FVM_Object *function, FVM_Object *error, bool loaded)
{
    const char *names[] = {"call", "apply", "toString"};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned flags = 99;
        assert(fvm_object_property_flags(c, i == 2 ? error : function, names[i], &flags) == FVM_OK && flags == (loaded ? i == 2 ? 0U : 3U : 1U));
    }
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; version += 2)
    {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 3000 && !success; ++boundary)
        {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Value klass, fn, error;
            assert(fvm_get_global(c, "Function", &klass) == FVM_OK && fvm_object_get(c, klass.object, "prototype", &fn) == FVM_OK);
            assert(fvm_get_global(c, "Error", &klass) == FVM_OK && fvm_object_get(c, klass.object, "prototype", &error) == FVM_OK);
            activation_flags(c, fn.object, error.object, false);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            allowance = boundary;
            FVM_Result status = fvm_load_memory(c, swf, sizeof(swf));
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(!c->swf_loaded); activation_flags(c, fn.object, error.object, false);
                assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
            }
            else { assert(status == FVM_OK); success = true; }
            activation_flags(c, fn.object, error.object, true);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}

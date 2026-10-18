/* Native StopSounds failure injection preserves roots and never rolls back delivered host stops. */
#define main stop_allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_STOP_SOUNDS_FIXTURE_ONLY
#include "jit_stop_sounds.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        if (loaded)
        {
            const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        Program p; p.number(42); p.op(9); p.finish();
        FVM_Object *fn = 0; FVM_JitCode *code = 0;
        assert(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK);
        assert(fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK);
        assert(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK);
        AudioHost host; register_audio(c, host, 1); host.probe = true;
        bool succeeded = false; unsigned failures = 0;
        for (long boundary = 0; boundary < 1000 && !succeeded; ++boundary)
        {
            activate(host); order_count = 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Value out; allowance = boundary;
            FVM_Result status = fvm_function_invoke(c, fn, 0, 0, 0, &out);
            allowance = -1;
            assert(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth);
            assert(!c->avm1_execution_depth && !code->active && !c->audio_busy);
            assert(order_count <= 1);
            active(host, order_count == 0);
            if (status == FVM_OK)
            {
                succeeded = true;
                assert(order_count == 1 && out.type == FVM_VALUE_NUMBER && out.number == 42);
            }
            else
            {
                assert(status == FVM_OUT_OF_MEMORY); ++failures;
                assert(fvm_gc_collect(c, 0) == FVM_OK);
            }
        }
        assert(succeeded && failures);
        /* The lowered helper itself needs no allocations, including the complete guarded stop. */
        activate(host); order_count = 0;
        FVM_JitInvocation state; state.context = c;
        allowance = 0;
        FVM_Result status = fvm_jit_stop_sounds(&state, 0);
        allowance = -1;
        assert(status == FVM_OK && order_count == 1 && !c->audio_busy && !c->gc_native_depth);
        active(host, false);
        assert(fvm_audio_channel_close(host.channel) == FVM_OK);
        assert(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt); return 0;
}

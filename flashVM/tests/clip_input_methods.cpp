/* Input methods preserve source gates, defining owner, getter cardinality and old API behavior. */
#include "clip_input_methods_fixture.hpp"
/* Receiver/owner identities and lookup counts for one method route. */
struct Probe
{
    FVM_Object *self; /* Borrowed receiver retained by the selected scene. */
    FVM_Object *owner; /* Defining prototype or own receiver. */
    FVM_Object *function; /* Getter result rooted by the accessor. */
    unsigned calls; /* Actual method entries. */
    unsigned gets; /* Getter entries. */
};
/* Verify inherited method identity and the event/collector/reentry boundaries. */
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned count,
                          const FVM_Value *, FVM_Value *out, void *raw)
{
    Probe *probe = static_cast<Probe *>(raw);
    assert(self == probe->self && c->calling_owner == probe->owner && !count);
    assert(c->clip_input_dispatching && c->stage_update_allowed);
    assert(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    unsigned long reclaimed = 77;
    assert(fvm_gc_step(c, &reclaimed) == FVM_OK && !reclaimed);
    assert(fvm_movieclip_dispatch_input(c, self, 8) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_dispatch_clip_input(c, self, 8) == FVM_INVALID_ARGUMENT);
    ++probe->calls;
    return fvm_call(c, "updateAfterEvent", 0, 0, out);
}
/* Return the single retained function from an inherited getter. */
static FVM_Result lookup(FVM_Context *, FVM_Object *self, unsigned count,
                         const FVM_Value *, FVM_Value *out, void *raw)
{
    Probe *probe = static_cast<Probe *>(raw);
    assert(self == probe->self && !count);
    ++probe->gets;
    *out = method_value(probe->function);
    return FVM_OK;
}
/* A missing member must never invoke a same-name global function. */
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned,
                            const FVM_Value *, FVM_Value *, void *)
{
    assert(false);
    return FVM_ERROR;
}
/* Exercise own/inherited/getter/native-method routes for all five source-version policies. */
int main()
{
    const char *names[] = {"onMouseMove", "onMouseDown", "onMouseUp", "onKeyDown", "onKeyUp"};
    const unsigned long events[] = {8, 16, 32, 64, 128};
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
        for (unsigned route = 0; route < 4; ++route)
        {
            FVM_Context *c = clip_context(runtime);
            FVM_Object *root = method_root(c, version), *proto = 0;
            Probe probe = {root, root, 0, 0, 0};
            assert(fvm_object_create(c, &proto) == FVM_OK);
            assert(fvm_object_set_prototype(c, root, proto) == FVM_OK);
            probe.owner = route == 3 ? 0 : route ? proto : root;
            assert(fvm_function_create_native(c, receive, &probe, 0, &probe.function) == FVM_OK);
            FVM_Value fn = method_value(probe.function);
            for (unsigned i = 0; i < 5; ++i)
            {
                if (route == 2)
                    assert(fvm_object_define_accessor_rooted(c, proto, names[i], lookup, 0, &probe, probe.function) == FVM_OK);
                else if (route == 3)
                    assert(fvm_object_define_method_rooted(c, proto, names[i], receive, &probe, 0) == FVM_OK);
                else
                    assert(fvm_object_set(c, probe.owner, names[i], &fn) == FVM_OK);
                assert(fvm_movieclip_dispatch_clip_input(c, root, events[i]) == FVM_OK && !probe.calls);
            }
            for (unsigned i = 0; i < 5; ++i)
                assert(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK);
            assert(probe.calls == (version >= 6 ? 5U : 0U));
            assert(probe.gets == (version >= 6 && route == 2 ? 5U : 0U));
            assert(c->stage_update_pending == (version >= 6));
            input_clean(c);
            fvm_context_destroy(c);
        }
    FVM_Context *c = clip_context(runtime);
    FVM_Object *root = method_root(c);
    for (unsigned i = 0; i < 5; ++i)
    {
        assert(fvm_register_function(c, names[i], forbidden, 0) == FVM_OK);
        assert(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK);
        FVM_Value scalar;
        fvm_value_number(&scalar, 17);
        assert(fvm_object_set(c, root, names[i], &scalar) == FVM_OK);
        assert(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK);
    }
    assert(fvm_movieclip_dispatch_input(c, root, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_dispatch_input(c, root, 24) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_dispatch_input(c, root, 1) == FVM_INVALID_ARGUMENT);
    input_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}

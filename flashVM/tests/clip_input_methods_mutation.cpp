/* Retained records precede method lookup; removed branches skip methods and newborns wait. */
#include "clip_input_methods_fixture.hpp"
/* Borrow the selected clip and count callbacks independently of method resolution. */
struct Mutation
{
    FVM_Object *root; /* Pinned scene root. */
    FVM_Object *actor; /* Original receiver whose retained record runs. */
    FVM_Object *added; /* Newly duplicated receiver, excluded from the first batch. */
    unsigned records; /* Committed retained callbacks. */
    unsigned methods; /* Committed optional method callbacks. */
    bool remove; /* Remove the receiver in its record rather than install a method. */
};
/* A method installed by the preceding retained record must be resolved at the same turn. */
static FVM_Result method(FVM_Context *, FVM_Object *, unsigned count,
                         const FVM_Value *, FVM_Value *out, void *raw)
{
    Mutation *mutation = static_cast<Mutation *>(raw);
    assert(!count);
    ++mutation->methods;
    fvm_value_undefined(out);
    return FVM_OK;
}
/* Change scene and handler membership from real retained AVM1 bytecode. */
static FVM_Result record(FVM_Context *c, FVM_Object *self, unsigned count,
                         const FVM_Value *, FVM_Value *out, void *raw)
{
    Mutation *mutation = static_cast<Mutation *>(raw);
    assert(!count);
    ++mutation->records;
    if (mutation->remove)
        return fvm_object_call(c, self, "removeMovieClip", 0, 0, out);
    assert(fvm_object_define_method_rooted(c, self, "onMouseMove", method, mutation, 0) == FVM_OK);
    if (!mutation->added)
    {
        mutation->added = input_duplicate(c, mutation->actor, "newborn", 5);
        assert(fvm_object_define_method_rooted(c, mutation->added, "record", record, mutation, 0) == FVM_OK);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}
/* Check both per-receiver snapshots and whole-scene recipient snapshots. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    for (unsigned remove = 0; remove < 2; ++remove)
    {
        FVM_Context *c = clip_context(runtime);
        Bytes code;
        integer(code, 0); variable(code, "this"); text(code, "record");
        code.push_back(0x52); code.push_back(0x17); code.push_back(0);
        FVM_Object *root = clip_load(c, clip_movie(8, clip_event_table(8, 8, code)));
        assert(fvm_movieclip_advance(c, root, 1) == FVM_OK);
        FVM_Object *actor = get(c, root, "actor").object;
        Mutation mutation = {root, actor, 0, 0, 0, remove != 0};
        assert(fvm_object_define_method_rooted(c, actor, "record", record, &mutation, 0) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, actor, "onMouseMove", method, &mutation, 0) == FVM_OK);
        assert(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK);
        assert(mutation.records == 1 && mutation.methods == (remove ? 0U : 1U));
        if (!remove)
        {
            assert(mutation.added);
            assert(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK);
            assert(mutation.records == 3 && mutation.methods == 3);
        }
        input_clean(c);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}

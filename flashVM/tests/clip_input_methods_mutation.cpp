/* Retained records precede method lookup; removed branches skip methods and newborns wait. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_methods_fixture.hpp"


/* Borrow the selected clip and count callbacks independently of method resolution. */
struct Mutation
{
    FVM_Object *root;  /* Pinned scene root. */
    FVM_Object *actor; /* Original receiver whose retained record runs. */
    FVM_Object *added; /* Newly duplicated receiver, excluded from the first batch. */
    unsigned records;  /* Committed retained callbacks. */
    unsigned methods;  /* Committed optional method callbacks. */
    bool remove;       /* Remove the receiver in its record rather than install a method. */
};


/* A method installed by the preceding retained record must be resolved at the same turn. */
static FVM_Result method(FVM_Context *, FVM_Object *, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Mutation *mutation = static_cast<Mutation *>(raw);
    if (!(!count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++mutation->methods;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Change scene and handler membership from real retained AVM1 bytecode. */
static FVM_Result record(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Mutation *mutation = static_cast<Mutation *>(raw);
    if (!(!count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++mutation->records;
    if (mutation->remove)
    {
        return fvm_object_call(c, self, "removeMovieClip", 0, 0, out);
    }
    if (!(fvm_object_define_method_rooted(c, self, "onMouseMove", method, mutation, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!mutation->added)
    {
        mutation->added = input_duplicate(c, mutation->actor, "newborn", 5);
        if (!(fvm_object_define_method_rooted(c, mutation->added, "record", record, mutation, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Check both per-receiver snapshots and whole-scene recipient snapshots. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned remove = 0; remove < 2; ++remove)
    {
        FVM_Context *c = clip_context(runtime);
        Bytes code;
        integer(code, 0);
        variable(code, "this");
        text(code, "record");
        code.push_back(0x52);
        code.push_back(0x17);
        code.push_back(0);
        FVM_Object *root = clip_load(c, clip_movie(8, clip_event_table(8, 8, code)));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object;
        Mutation mutation = {root, actor, 0, 0, 0, remove != 0};
        if (!(fvm_object_define_method_rooted(c, actor, "record", record, &mutation, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, actor, "onMouseMove", method, &mutation, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(mutation.records == 1 && mutation.methods == (remove ? 0U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!remove)
        {
            if (!(mutation.added))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(mutation.records == 3 && mutation.methods == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        input_clean(c);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}

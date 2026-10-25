/* Shared public construction helpers for insertion-independent native timeline order. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_fixture.hpp"
#include "../src/standard/movieclip_storage.hpp"


/* Create a signed-depth child with ordinary native script-visible construction. */
static FVM_Object *ordered_child(FVM_Context *c, FVM_Object *root, const char *name, long depth)
{
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, (double)depth);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


/* Observe exact receiver identities without allocating inside the callback. */
struct OrderTrace
{
    FVM_Object *expected[8]; /* Borrowed receivers retained by the pinned root/barrier. */
    unsigned count;          /* Number of committed callbacks. */
    unsigned total;          /* Expected callback count. */
};


/* The callback ordering oracle is independent of the native child-table representation. */
static FVM_Result ordered(FVM_Context *, FVM_Object *self, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    OrderTrace *trace = static_cast<OrderTrace *>(raw);
    if (!(!count && trace->count < trace->total && self == trace->expected[trace->count]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++trace->count;
    fvm_value_undefined(out);
    return FVM_OK;
}

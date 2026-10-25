/* Removing a parent in its callback suppresses descendants whose physical parent edge still exists. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_order_fixture.hpp"


/* The removed receiver itself has already committed; its descendants have not. */
struct Removal
{
    unsigned parents;     /* Number of remover entries. */
    unsigned descendants; /* Must remain zero after effective ancestor removal. */
};


/* Detach this branch through the public method after its own onEnterFrame begins. */
static FVM_Result remove_self(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++static_cast<Removal *>(raw)->parents;
    return fvm_object_call(c, self, "removeMovieClip", 0, 0, out);
}


/* This method cannot run while a captured native ancestor is effectively removed. */
static FVM_Result descendant(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++static_cast<Removal *>(raw)->descendants;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* A selected removed root retains the old explicit-call policy, but detached descendants skip. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = tick_context(runtime);
    FVM_Object *root = tick_root(c);
    FVM_Object *parent = ordered_child(c, root, "parent", 1), *child = ordered_child(c, parent, "child", 1);
    Removal removal = {0, 0};
    if (!(fvm_object_define_method_rooted(c, parent, "onEnterFrame", remove_self, &removal, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, child, "onEnterFrame", descendant, &removal, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && removal.parents == 1 && !removal.descendants))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_storage::parent(child) == parent))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && removal.parents == 1 && !removal.descendants))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}

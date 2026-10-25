/* World queries preserve outputs/root state on cycles and getter exceptions under native GC barriers. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_storage.hpp"

static unsigned fail, calls;


static FVM_Result getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 0);
    return fail ? fvm_exception_raise(c, out) : FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = 0;
    FVM_Object *root = 0, *leaf = 0, *data = 0;
    FVM_Value a[2], value, parent;
    FVM_TwipMatrix out = {7, 8, 9, 10, 77, 88};
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(a, "leaf");
    fvm_value_number(a + 1, 1);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    leaf = value.object;
    if (!(fvm_movieclip_storage::state(root, data) == FVM_OK &&
          fvm_movieclip_storage::field(data, "parent", parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data->properties["parent"] = fvm_movieclip_storage::object(leaf);
    FVM_GCRootFrame *roots = c->gc_frames;
    if (!(fvm_exec_clip_world_matrix(c, leaf, &out) == FVM_INVALID_ARGUMENT && out.a == 7 && out.b == 8 && out.c == 9 &&
          out.d == 10 && out.tx == 77 && out.ty == 88 && c->gc_frames == roots && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data->properties["parent"] = parent;
    int deleted = 0;
    if (!(fvm_object_delete(c, root, "_x", &deleted) == FVM_OK && deleted &&
          fvm_object_define_accessor_rooted(c, root, "_x", getter, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail = 1;
    if (!(fvm_exec_clip_world_matrix(c, leaf, &out) == FVM_EXCEPTION && calls == 1 && out.tx == 77 && out.a == 7 &&
          fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail = 0;
    if (!(fvm_exec_clip_world_matrix(c, leaf, &out) == FVM_OK && calls == 2 && out.a == 1 && out.tx == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}

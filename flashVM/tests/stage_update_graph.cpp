/* Imported updateAfterEvent Functions use destination event eligibility and no exported redraw state. */
#include <stdio.h>
#include <stdlib.h>
#include "timers_fixture.hpp"
#include "flashvm/serialization.h"
#include "flashvm/stage.h"
#include "flashvm/mouse.h"


struct Export
{
    FVM_Value function;
    unsigned char *bytes;
    size_t size;


    Export() : bytes(0), size(0)
    {
    }
};


static FVM_Result export_inside(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Export *s = (Export *)raw;
    if (!(c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, s->function.object, 0, 0, 0, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_value_serialize(c, &s->function, &s->bytes, &s->size);
}


static FVM_Result invoke_imported(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Export *s = (Export *)raw;
    if (!(c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_function_invoke(c, s->function.object, 0, 0, 0, out);
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
    FVM_Context *a = timer_context(rt);
    Export state;
    if (!(fvm_get_global(a, "updateAfterEvent", &state.function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_numeric_callback_id(state.function.object->invocable.callback) == 168 &&
          fvm_numeric_callback_from_id(168) == state.function.object->invocable.callback))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *fn = 0;
    if (!(fvm_function_create_native(a, export_inside, &state, state.function.object, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[] = {timer_object(fn), timer_number(1)};
    timer_call(a, "setInterval", 2, args);
    if (!(timer_poll(a, 1001) == 1 && a->stage_update_pending && !a->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(a);
    FVM_Context *b = timer_context(rt);
    if (!(fvm_value_deserialize(b, state.bytes, state.size, &state.function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(state.bytes);
    if (!(!b->stage_update_pending && !b->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value ignored;
    if (!(fvm_function_invoke(b, state.function.object, 0, 0, 0, &ignored) == FVM_OK && !b->stage_update_pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *listener = 0;
    if (!(fvm_object_create(b, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(b, listener, "onMouseMove", invoke_imported, &state, state.function.object) ==
              FVM_OK &&
          fvm_input_add_mouse_listener(b, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(b, 0) == FVM_OK && fvm_input_dispatch_mouse(b, FVM_MOUSE_EVENT_MOVE, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int pending = 0;
    if (!(fvm_stage_take_update(b, &pending) == FVM_OK && pending == 1 && !b->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(b);
    fvm_runtime_destroy(rt);
}

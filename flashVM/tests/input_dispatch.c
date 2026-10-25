#include <stdio.h>
#include <stdlib.h>
#include "flashvm/flashvm.h"
#include "flashvm/input.h"

static int called = 0;


static void
on_input(FVM_Context *ctx, const FVM_InputEvent *event, FVM_InputPhase phase, FVM_MouseButton button, void *userdata)
{
    (void)ctx;
    (void)userdata;
    if (!(event->fingers == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(phase == FVM_INPUT_END))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(button == FVM_MOUSE_RIGHT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++called;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *ctx = 0;
    FVM_InputRule rule;
    FVM_InputEvent event;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &ctx) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rule.struct_size = sizeof(rule);
    rule.gesture = FVM_GESTURE_TAP;
    rule.fingers = 2;
    rule.direction = 0;
    rule.button = FVM_MOUSE_RIGHT;
    if (!(fvm_input_set_rule(ctx, &rule) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.gesture = FVM_GESTURE_TAP;
    event.fingers = 2;
    event.direction = 0;
    event.x = 20;
    event.y = 30;
    if (!(fvm_input_set_callback(ctx, on_input, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch(ctx, &event, FVM_INPUT_END) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(called == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(runtime);
    return 0;
}

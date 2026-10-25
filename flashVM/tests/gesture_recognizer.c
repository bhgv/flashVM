#include <stdio.h>
#include <stdlib.h>
#include "flashvm/gesture_recognizer.h"

static int calls = 0;


static void on_event(FVM_Context *ctx, const FVM_InputEvent *e, FVM_InputPhase phase, FVM_MouseButton button, void *ud)
{
    (void)ctx;
    (void)ud;
    if (!(phase == FVM_INPUT_END))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (calls == 0)
    {
        if (!(e->gesture == FVM_GESTURE_TAP))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(e->fingers == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(button == FVM_MOUSE_RIGHT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(e->gesture == FVM_GESTURE_SWIPE))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(e->direction == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(button == FVM_MOUSE_WHEEL_DOWN))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    ++calls;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    FVM_GestureRecognizer rec;
    FVM_InputRule rule;
    int dispatched = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &ctx) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_callback(ctx, on_event, 0) == FVM_OK))
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
    rule.gesture = FVM_GESTURE_SWIPE;
    rule.direction = 1;
    rule.button = FVM_MOUSE_WHEEL_DOWN;
    if (!(fvm_input_set_rule(ctx, &rule) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rec.struct_size = sizeof(rec);
    if (!(fvm_gesture_recognizer_init(&rec) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_BEGIN, 1, 0, 0, 100, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_MOVE, 2, 1, 1, 120, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_END, 0, 2, 2, 150, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dispatched == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_BEGIN, 2, 0, 0, 200, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_END, 0, 100, 0, 400, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dispatched == 1 && calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_BEGIN, 1, 0, 0, 500, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gesture_recognizer_feed(ctx, &rec, FVM_TOUCH_CANCEL, 0, 0, 0, 510, &dispatched) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return 0;
}

/* C89 immediate and queued input share bounds and reject malformed events without side effects. */
#include <stdio.h>
#include "flashvm/input_queue.h"
#include <math.h>
#include <stdlib.h>

static unsigned calls;


static void receive(FVM_Context *c, const FVM_InputEvent *e, FVM_InputPhase phase, FVM_MouseButton button, void *raw)
{
    (void)c;
    (void)raw;
    (void)button;
    if (!(e->fingers && e->fingers <= FVM_INPUT_MAX_FINGERS && phase >= FVM_INPUT_BEGIN && phase <= FVM_INPUT_END))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
}


static void invalid(FVM_Context *c, const FVM_InputEvent *event)
{
    FVM_MouseButton button = FVM_MOUSE_RIGHT;
    unsigned pending = 0, before = calls;
    if (!(fvm_input_map(c, event, &button) == FVM_INVALID_ARGUMENT && button == FVM_MOUSE_RIGHT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch(c, event, FVM_INPUT_END) == FVM_INVALID_ARGUMENT && calls == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue(c, event, FVM_INPUT_END) == FVM_INVALID_ARGUMENT &&
          fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_InputEvent event, bad;
    FVM_InputRule rule;
    unsigned pending, i, before, processed;
    int gesture, phase;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_input_set_callback(c, receive, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.gesture = FVM_GESTURE_TAP;
    event.fingers = 1;
    event.direction = 0;
    event.x = 7;
    event.y = 9;
    if (!(fvm_input_enqueue(c, &event, FVM_INPUT_END) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid(c, 0);
    bad = event;
    bad.fingers = 0;
    invalid(c, &bad);
    bad.fingers = FVM_INPUT_MAX_FINGERS + 1;
    invalid(c, &bad);
    bad = event;
    bad.gesture = (FVM_Gesture)0;
    invalid(c, &bad);
    bad.gesture = (FVM_Gesture)4;
    invalid(c, &bad);
    bad = event;
    bad.x = HUGE_VAL;
    invalid(c, &bad);
    bad.x = -HUGE_VAL;
    invalid(c, &bad);
    bad = event;
    bad.y = HUGE_VAL;
    invalid(c, &bad);
    bad.y = -HUGE_VAL;
    invalid(c, &bad);
    bad = event;
    bad.x = strtod("NaN", 0);
    if (!(bad.x != bad.x))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid(c, &bad);
    bad = event;
    bad.y = strtod("NaN", 0);
    invalid(c, &bad);
    before = calls;
    if (!(fvm_input_dispatch(c, &event, (FVM_InputPhase)0) == FVM_INVALID_ARGUMENT &&
          fvm_input_enqueue(c, &event, (FVM_InputPhase)4) == FVM_INVALID_ARGUMENT && calls == before &&
          fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &processed) == FVM_OK && processed == 1 && calls == before + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rule.struct_size = sizeof(rule);
    rule.gesture = FVM_GESTURE_TAP;
    rule.fingers = FVM_INPUT_MAX_FINGERS;
    rule.direction = 0;
    rule.button = FVM_MOUSE_LEFT;
    if (!(fvm_input_set_rule(c, &rule) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++rule.fingers;
    if (!(fvm_input_set_rule(c, &rule) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = calls;
    for (gesture = 1; gesture <= 3; ++gesture)
    {
        for (i = 1; i <= FVM_INPUT_MAX_FINGERS; ++i)
        {
            for (phase = 1; phase <= 3; ++phase)
            {
                event.gesture = (FVM_Gesture)gesture;
                event.fingers = i;
                if (!(fvm_input_dispatch(c, &event, (FVM_InputPhase)phase) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    if (!(calls == before + 3 * FVM_INPUT_MAX_FINGERS * 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = calls;
    for (i = 0; i < FVM_INPUT_MAX_PENDING_EVENTS; ++i)
    {
        if (!(fvm_input_enqueue(c, &event, FVM_INPUT_UPDATE) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_input_enqueue(c, &event, FVM_INPUT_UPDATE) == FVM_ERROR && fvm_input_pending(c, &pending) == FVM_OK &&
          pending == FVM_INPUT_MAX_PENDING_EVENTS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 1, &processed) == FVM_OK && processed == 1 &&
          fvm_input_enqueue(c, &event, FVM_INPUT_UPDATE) == FVM_OK && fvm_input_pump(c, 0, &processed) == FVM_OK &&
          processed == FVM_INPUT_MAX_PENDING_EVENTS && calls == before + FVM_INPUT_MAX_PENDING_EVENTS + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}

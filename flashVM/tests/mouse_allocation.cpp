/* Reuse the controlled native allocator to check Mouse installation, registration and dispatch. */
#define main selection_install_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "selection_install_allocation.cpp"

#undef main
#include "flashvm/mouse.h"
#include "flashvm/input_queue.h"
#include "../src/standard/mouse_listeners.hpp"


int main()
{
    bool done = false;
    unsigned failed = 0;
    for (long allocation = 0; allocation < 512 && !done; ++allocation)
    {
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        FVM_Context *c = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_after = allocation;
        FVM_Result r = fvm_standard_mouse_install(c);
        fail_after = -1;
        if (r == FVM_OK)
        {
            done = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failed;
            if (!(c->global->properties.find("Mouse") == c->global->properties.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    if (!(done && failed > 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *listener = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_object_create(c, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    if (!(fvm_input_add_mouse_listener(c, listener) == FVM_OUT_OF_MEMORY && c->mouse_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(fvm_input_add_mouse_listener(c, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    done = false;
    unsigned before = 0, after = 0;
    for (long allocation = 0; allocation < 128 && !done; ++allocation)
    {
        if (!(fvm_input_set_pointer(c, 3, 4) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_after = allocation;
        FVM_Result r = fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 8, 9);
        fail_after = -1;
        double x, y;
        if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            done = true;
            if (!(x == 8 && y == 9))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (x == 3)
            {
                if (!(y == 4))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++before;
            }
            else
            {
                if (!(x == 8 && y == 9))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++after;
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(done && before && after))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    done = false;
    before = after = 0;
    for (long allocation = 0; allocation < 128 && !done; ++allocation)
    {
        if (!(fvm_input_set_pointer(c, 3, 4) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_after = allocation;
        FVM_Result status = fvm_input_dispatch_mouse_wheel(c, -2.5, 18, 19);
        fail_after = -1;
        double x, y;
        if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (status == FVM_OK)
        {
            done = true;
            if (!(x == 18 && y == 19))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(status == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (x == 3)
            {
                if (!(y == 4))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++before;
            }
            else
            {
                if (!(x == 18 && y == 19))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++after;
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(done && before && after))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    if (!(fvm_input_enqueue_mouse_wheel(c, 1.25, 7, 8) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    unsigned pending = 99, growth_failures = 0;
    if (!(fvm_input_pending(c, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, 1.25, 7, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 1; i < FVM_INPUT_MAX_PENDING_EVENTS; ++i)
    {
        fail_after = 0;
        FVM_Result status = fvm_input_enqueue_mouse_wheel(c, -0.5, i, 9);
        fail_after = -1;
        if (status == FVM_OUT_OF_MEMORY)
        {
            ++growth_failures;
            if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == i))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->queued_input.front().wheel_delta == 1.25 && c->queued_input.front().event.x == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_enqueue_mouse_wheel(c, -0.5, i, 9) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(status == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(growth_failures && fvm_input_pending(c, &pending) == FVM_OK && pending == FVM_INPUT_MAX_PENDING_EVENTS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, 1, 0, 0) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_clear_queue(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    int previous = -1, visible = -1;
    if (!(fvm_input_set_cursor_visible(c, 0, &previous) == FVM_OK && previous == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_cursor_visible(c, &visible) == FVM_OK && visible == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(c, 1, &previous) == FVM_OK && previous == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value cursor_result;
    if (!(fvm_mouse_hide(c, 0, 0, 0, &cursor_result, 0) == FVM_OK && cursor_result.number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_mouse_show(c, 0, 0, 0, &cursor_result, 0) == FVM_OK && cursor_result.number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    fail_after = 0;
    int removed = 0;
    if (!(fvm_input_remove_mouse_listener(c, listener, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}

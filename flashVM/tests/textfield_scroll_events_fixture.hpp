/* Shared TextField scroll event registration and exact receiver/argument capture. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_TEXTFIELD_SCROLL_EVENTS_FIXTURE_HPP
#define FVM_TEST_TEXTFIELD_SCROLL_EVENTS_FIXTURE_HPP
#include "textfield_scroll_fixture.hpp"
#include "../src/standard/text_listeners.hpp"

#ifdef FVM_HAS_FONTS
struct ScrollEvents
{
    FVM_Object *field;
    unsigned trace[128], count;
    unsigned long top, horizontal;
};


struct ScrollObserver
{
    ScrollEvents *events;
    unsigned id;
};


static FVM_Result
scroll_observe(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    ScrollObserver *observer = (ScrollObserver *)raw;
    ScrollEvents *events = observer->events;
    if (!(n == 1 && args[0].type == FVM_VALUE_OBJECT && args[0].object == events->field && events->count < 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldScrollInfo info = scroll_info(c, events->field);
    if (!(info.scroll == events->top && info.hscroll == events->horizontal))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->gc_frames && c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events->trace[events->count++] = observer->id;
    fvm_value_undefined(out);
    return FVM_OK;
}


static bool scroll_listen(FVM_Context *c, FVM_Object *field, FVM_Object *listener, const char *method = "addListener")
{
    FVM_Value arg, out;
    fvm_value_undefined(&arg);
    arg.type = listener ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    arg.object = listener;
    if (!(fvm_object_call(c, field, method, 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.boolean != 0;
}


static FVM_Object *scroll_observer(FVM_Context *c, ScrollObserver *data)
{
    FVM_Object *object = 0;
    if (!(fvm_object_create(c, &object) == FVM_OK &&
          fvm_object_define_method_rooted(c, object, "onScroller", scroll_observe, data, data->events->field) ==
              FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return object;
}
#endif
#endif

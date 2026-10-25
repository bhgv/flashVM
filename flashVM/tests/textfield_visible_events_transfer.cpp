/* Counted text notifications survive binary/channel graph and font ownership after sender teardown. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#ifdef FVM_HAS_FONTS


static FVM_Object *script_listener(FVM_Context *c)
{
    FVM_Object *object = 0, *fn = 0;
    if (!(fvm_object_create(c, &object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    fvm_value_number(&v, 0);
    if (!(fvm_object_set(c, object, "seen", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program p;
    p.variable("this");
    p.text("seen");
    p.variable("this");
    p.text("seen");
    p.op(0x4e);
    p.number(1);
    p.op(0x47);
    p.op(0x4f);
    p.variable("this");
    p.text("source");
    p.variable("arguments");
    p.number(0);
    p.op(0x4e);
    p.op(0x4f);
    p.number(0);
    p.finish();
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = class_value(fn);
    if (!(fvm_object_set(c, object, "onScroller", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return object;
}
#endif
#ifndef FVM_SCROLL_EVENTS_TRANSFER_FIXTURE_ONLY


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *d = 0, *e = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK &&
          fvm_context_create(rt, &e) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    scroll_number(c, field, "_width", 60);
    scroll_number(c, field, "_height", 200);
    FVM_Object *listener = script_listener(c);
    if (!(scroll_listen(c, field, listener)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = class_value(listener);
    if (!(fvm_object_set(c, field, "observer", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value root = class_value(field), out;
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK && fvm_standard_text_install(d) == FVM_OK &&
          fvm_value_deserialize(d, wire, size, &out) == FVM_OK && fvm_gc_pin(d, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *copy = out.object;
    fvm_serialized_free(wire);
    FVM_Channel *channel = 0;
    if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &root, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(fvm_standard_text_install(e) == FVM_OK && fvm_channel_receive_value(e, channel, &out, 0) == FVM_OK &&
          fvm_gc_pin(e, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    FVM_Context *contexts[] = {d, e};
    FVM_Object *fields[] = {copy, out.object};
    for (unsigned j = 0; j < 2; ++j)
    {
        if (!(fvm_gc_collect(contexts[j], 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(edit_text(contexts[j], fields[j], "A", 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(scroll_info(contexts[j], fields[j]).bottom_scroll == 1 &&
              scroll_info(contexts[j], fields[j]).maxscroll == 1 && !scroll_info(contexts[j], fields[j]).maxhscroll))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(contexts[j], fields[j], "observer", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *observer = value.object;
        if (!(counted_metric(contexts[j], observer, "seen") == 1 &&
              fvm_object_get(contexts[j], observer, "source", &value) == FVM_OK && value.object == fields[j]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(scroll_listen(contexts[j], fields[j], observer, "removeListener")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(edit_text(contexts[j], fields[j], "AV\nAV\nAV", 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(counted_metric(contexts[j], observer, "seen") == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!contexts[j]->gc_frames && !contexts[j]->gc_native_depth && !contexts[j]->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(d);
    fvm_context_destroy(e);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

#endif

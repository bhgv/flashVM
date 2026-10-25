/* Legacy native method upgrades preserve saved unsupported aliases and captured host callbacks. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_fixture.hpp"
#include "flashvm/function.h"

#ifdef FVM_HAS_FONTS
static FVM_Result
captured_override(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    if (!(raw == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(out, 0);
    return FVM_OK;
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *d = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK &&
          fvm_standard_install(c) == FVM_OK && fvm_standard_install(d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    const char *names[] = {"addListener", "removeListener"};
    for (unsigned i = 0; i < 2; ++i)
    {
        FVM_NativeEntry old;
        old.callback = fvm_text_callback_from_id(FVM_TEXT_LISTENER_RETIRED_CALLBACK);
        field->methods[names[i]] = old;
    }
    FVM_Value alias, value, root, out;
    if (!(fvm_object_get(c, field, "addListener", &alias) == FVM_OK && alias.type == FVM_VALUE_OBJECT &&
          fvm_object_set(c, field, "savedRetiredAlias", &alias) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK && fvm_standard_text_install(d) == FVM_OK &&
          fvm_value_deserialize(d, wire, size, &out) == FVM_OK && fvm_gc_pin(d, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    FVM_Object *copy = out.object, *upgraded = out.object, *listener = 0;
    if (!(fvm_object_create(d, &listener) == FVM_OK && scroll_listen(d, copy, listener)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(d, copy, "savedRetiredAlias", &alias) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = listener;
    if (!(fvm_function_invoke(d, alias.object, copy, 1, &value, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, field, "addListener", captured_override, field, field) == FVM_OK &&
          fvm_serialization_register_callback(c, "text-listener-capture", captured_override) == FVM_OK &&
          fvm_serialization_register_callback(d, "text-listener-capture", captured_override) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK &&
          fvm_value_deserialize(d, wire, size, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    copy = out.object;
    if (!(copy->methods["addListener"].callback == captured_override && copy->methods["addListener"].root == copy &&
          copy->methods["addListener"].userdata == copy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!scroll_listen(d, copy, listener)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(scroll_listen(d, upgraded, upgraded) && fvm_gc_unpin(d, upgraded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long reclaimed = 0;
    if (!(fvm_gc_collect(d, &reclaimed) == FVM_OK && reclaimed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < d->objects.size(); ++i)
    {
        if (!(d->objects[i] != upgraded))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(d, 0) == FVM_OK && !d->gc_frames && !d->gc_native_depth && !d->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_context_destroy(d);
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *owner = 0;
        if (!(fvm_context_create(rt, &owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char movie[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        movie[3] = (unsigned char)version;
        if (!(fvm_load_memory(owner, movie, sizeof(movie)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ScrollEvents events;
        events.field = scroll_field(owner);
        events.count = 0;
        events.top = 2;
        events.horizontal = 0;
        ScrollObserver own = {&events, 10}, listener_data = {&events, 20};
        if (!(fvm_object_define_method_rooted(owner, events.field, "onScroller", scroll_observe, &own, events.field) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *observer = scroll_observer(owner, &listener_data);
        FVM_Value argument;
        fvm_value_undefined(&argument);
        argument.type = FVM_VALUE_OBJECT;
        argument.object = observer;
        FVM_Result result = fvm_object_call(owner, events.field, "addListener", 1, &argument, &out);
        if (!(result == (version < 6 ? FVM_UNSUPPORTED : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scroll_number(owner, events.field, "scroll", 2);
        if (!(events.count == (version < 6 ? 0U : 2U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!owner->gc_frames && !owner->gc_native_depth && !owner->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(owner);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

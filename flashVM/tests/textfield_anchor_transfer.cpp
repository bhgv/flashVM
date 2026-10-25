/* Binary/channel receivers retain all anchors and signed origins after sender destruction. */
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        FVM_Context *c = 0, *d = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c);
        FVM_Value v, root, out;
        fvm_value_number(&v, -25);
        if (!(fvm_object_set(c, field, "_x", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 19);
        if (!(fvm_object_set(c, field, "_y", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "AV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *name = mode == 0 ? "left" : mode == 1 ? "center" : "right";
        if (!(fvm_value_string(&v, name) == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double x = counted_metric(c, field, "_x"), w = counted_metric(c, field, "_width");
        fvm_value_undefined(&root);
        root.type = FVM_VALUE_OBJECT;
        root.object = field;
        if (!(fvm_standard_text_install(d) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char *wire = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK &&
              fvm_value_deserialize(d, wire, size, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(wire);
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(d, copy) == FVM_OK && counted_metric(d, copy, "_x") == x &&
              counted_metric(d, copy, "_y") == 19))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(d, copy, "autoSize", &v) == FVM_OK && !std::strcmp(v.string, name)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Channel *ch = 0;
        if (!(fvm_channel_create(&ch) == FVM_OK && fvm_channel_send_value(c, ch, &root, 0) == FVM_OK &&
              fvm_channel_receive_value(d, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(counted_metric(d, out.object, "_x") == x && counted_metric(d, out.object, "_y") == 19))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        if (!(fvm_gc_collect(d, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "AVA") == FVM_OK && fvm_object_set(d, copy, "text", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double nw = counted_metric(d, copy, "_width");
        if (!(std::fabs(counted_metric(d, copy, "_x") - (x + (w - nw) * mode * 0.5)) < 1e-9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_TextFieldState state;
        if (!(fvm_textfield_snapshot(copy, state) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned bad = 0; bad < 5; ++bad)
        {
            FVM_TextFieldState invalid = state;
            FVM_Object *o = 0;
            if (bad == 0)
            {
                invalid.x = 1000001;
            }
            else if (bad == 1)
            {
                invalid.y = -1000001;
            }
            else if (bad == 2)
            {
                invalid.anchor = 3;
            }
            else if (bad == 3)
            {
                invalid.anchor = 1;
                invalid.auto_size_left = false;
            }
            else
            {
                invalid.x = HUGE_VAL;
            }
            if (!(fvm_object_create(d, &o) == FVM_OK && fvm_textfield_restore(d, o, invalid) == FVM_INVALID_ARGUMENT &&
                  !o->resource))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(d);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

/* Wrapped automatic fields preserve both horizontal edges for every anchor. */
#define main anchor_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_autosize_anchors.cpp"

#undef main


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c);
        FVM_Value v;
        put_number(c, field, "_x", -10);
        put_number(c, field, "_y", 25);
        put_number(c, field, "_width", 30);
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, field, "wordWrap", &v) == FVM_OK &&
              fvm_object_set(c, field, "multiline", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_text(c, field, "autoSize", mode == 0 ? "left" : mode == 1 ? "center" : "right");
        put_text(c, field, "text", "AV AV AV AV");
        if (!(counted_metric(c, field, "_width") == 30 && counted_metric(c, field, "_x") == -10 &&
              counted_metric(c, field, "_y") == 25))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double h = counted_metric(c, field, "_height");
        put_number(c, field, "_width", 20);
        if (!(counted_metric(c, field, "_x") == -10 && counted_metric(c, field, "_height") >= h))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 0);
        if (!(fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double w = counted_metric(c, field, "_width");
        if (!(std::fabs(counted_metric(c, field, "_x") - (-10 + (20 - w) * mode * 0.5)) < 1e-9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_text(c, field, "text", "");
        if (!(counted_metric(c, field, "_width") == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

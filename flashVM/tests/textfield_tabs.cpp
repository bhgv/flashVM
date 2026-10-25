/* Applied tab tables are copied; source/snapshot Array mutation cannot edit the native field. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_tabs_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c);
    double stops[] = {40, 80};
    FVM_Object *format = tabs_format(c, stops, 2);
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "A\tV\tA");
    inset_string(c, field, "autoSize", "left");
    if (!(counted_metric(c, field, "textWidth") == 104 && counted_metric(c, field, "_width") == 108 &&
          counted_metric(c, field, "_height") == 48))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value a, v, out;
    if (!(fvm_object_get(c, format, "tabStops", &a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 60);
    if (!(fvm_table_set_index(c, a.object, 0, &v) == FVM_OK && counted_metric(c, field, "textWidth") == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_string(c, field, "text", "A\tV\tA");
    if (!(counted_metric(c, field, "textWidth") == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
          fvm_object_get(c, out.object, "tabStops", &a) == FVM_OK && counted_metric(c, a.object, "length") == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_table_set_index(c, a.object, 0, &v) == FVM_OK && counted_metric(c, field, "textWidth") == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *p = 0;
    if (!(fvm_textfield_path(c, field, &p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(fvm_path_count(p)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

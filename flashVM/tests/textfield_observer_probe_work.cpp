/* Large SWF6 descriptor tables consume shared work before edits publish; no observer executes during probing. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"
#include <cstdio>

#ifdef FVM_HAS_FONTS
static FVM_Result
probe_work_callback(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++*(unsigned *)raw;
    fvm_value_undefined(out);
    return FVM_OK;
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char movie[] = {'F', 'W', 'S', 6, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    FVM_Value value;
    fvm_value_number(&value, 1);
    for (unsigned i = 0; i < 128; ++i)
    {
        char name[32];
        std::sprintf(name, "probe%03u", i);
        if (!(fvm_object_set(c, field, name, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    unsigned calls = 0;
    if (!(fvm_object_define_method_rooted(c, field, "ONSCROLLER", probe_work_callback, &calls, field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool present = true;
    rt->config.instruction_limit = 16;
    if (!(fvm_textfield_observer_present(c, field, &present) == FVM_ERROR && !present && !calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = 0;
    if (!(fvm_textfield_observer_present(c, field, &present) == FVM_OK && present && c->avm1_steps >= 128 && !calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned before = 0, after = 0, success = 0;
    for (unsigned limit = 1; limit <= 256; ++limit)
    {
        rt->config.instruction_limit = 0;
        if (!(edit_text(c, field, "AV\nAV\nAV", 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        calls = 0;
        if (!(fvm_value_string(&value, "A") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = limit;
        FVM_Result result = fvm_object_set(c, field, "text", &value);
        rt->config.instruction_limit = 0;
        if (!(result == FVM_OK || result == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double width = counted_metric(c, field, "textWidth");
        if (result == FVM_OK)
        {
            ++success;
            if (!(width == 24 && calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (width == 48)
        {
            ++before;
            if (!(!calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            counted_text(c, field, "AV\nAV\nAV", 8);
        }
        else
        {
            ++after;
            if (!(width == 24 && !calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(before && after && success && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

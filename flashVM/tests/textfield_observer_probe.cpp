/* Descriptor admission respects folded names/visibility/inheritance without evaluating getters. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"
#include "../src/core/property_version_constants.h"
#include "flashvm/property.h"

#ifdef FVM_HAS_FONTS


static FVM_Result probe_callback(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
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
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned versions[] = {0, 6, 7, 8};
    for (unsigned v = 0; v < 4; ++v)
    {
        for (unsigned kind = 0; kind < 3; ++kind)
        {
            for (unsigned folded = 0; folded < 2; ++folded)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (versions[v])
                {
                    unsigned char movie[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    movie[3] = (unsigned char)versions[v];
                    if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_Object *field = scroll_field(c), *prototype = 0;
                if (!(fvm_object_create(c, &prototype) == FVM_OK &&
                      fvm_object_set_prototype(c, field, prototype) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *name = folded ? "ONSCROLLER" : "onScroller";
                unsigned calls = 0;
                if (kind == 0)
                {
                    FVM_Value value;
                    fvm_value_boolean(&value, 0);
                    if (!(fvm_object_set(c, prototype, name, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (kind == 1)
                {
                    if (!(fvm_object_define_method_rooted(c, prototype, name, probe_callback, &calls, field) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_object_define_accessor_rooted(c, prototype, name, probe_callback, 0, &calls, field) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                bool present = false;
                const bool visible = !folded || versions[v] == 6;
                if (!(fvm_textfield_observer_present(c, field, &present) == FVM_OK && present == visible && !calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                prototype->attributes[name] |= FVM_PROPERTY_AVM1_VERSION_MASK;
                present = true;
                if (!(fvm_textfield_observer_present(c, field, &present) == FVM_OK &&
                      present == (visible && !versions[v]) && !calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *saved = field->prototype;
                field->prototype = field;
                present = true;
                if (!(fvm_textfield_observer_present(c, field, &present) == FVM_INVALID_ARGUMENT && !present))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                field->prototype = saved;
                if (!(fvm_textfield_observer_present(c, field, 0) == FVM_INVALID_ARGUMENT &&
                      fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
                      !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

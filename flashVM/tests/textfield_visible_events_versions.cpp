/* Bottom-only events keep SWF4/5 gates, suppress onChanged and reject foreign observer-probe graphs. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"

#ifdef FVM_HAS_FONTS
static FVM_Result
visible_unexpected_change(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(!"programmatic bottom-only edits must not synthesize onChanged"))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *foreign = 0;
    if (!(fvm_object_create(other, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned versions[] = {0, 4, 5, 6, 7, 8};
    for (unsigned version = 0; version < 6; ++version)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (versions[version])
            {
                unsigned char movie[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                movie[3] = (unsigned char)versions[version];
                if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            GeometryEvents events;
            events.field = scroll_field(c);
            events.count = 0;
            const bool modern = !versions[version] || versions[version] >= 6;
            if (mode == 2)
            {
                FVM_Value value;
                fvm_value_boolean(&value, 1);
                if (!(fvm_object_set(c, events.field, "wordWrap", &value) == FVM_OK &&
                      fvm_object_set(c, events.field, "autoSize", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                scroll_number(c, events.field, "_width", 60);
                scroll_number(c, events.field, "_height", 200);
            }
            GeometryObserver own = {&events, 1}, listener = {&events, 2};
            if (!(fvm_object_define_method_rooted(
                      c, events.field, "onScroller", geometry_observe, &own, events.field) == FVM_OK &&
                  fvm_object_define_method_rooted(
                      c, events.field, "onChanged", visible_unexpected_change, 0, events.field) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (modern)
            {
                geometry_listener(c, &listener);
            }
            bool present = true;
            if (!(fvm_textfield_observer_present(c, events.field, &present) == FVM_OK && present == modern))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *format = edit_format(c, 20);
            if (!((mode == 0   ? edit_text(c, events.field, "A", 1)
                   : mode == 1 ? edit_replace(c, events.field, 0, 8, "A", 1)
                               : edit_apply(c, events.field, format)) == FVM_OK &&
                  events.count == (modern ? 2U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
            if (!(info.scroll == 1 && !info.hscroll && info.maxscroll == 1 && !info.maxhscroll &&
                  info.bottom_scroll == (mode == 2 ? 3UL : 1UL)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            present = true;
            if (!(fvm_textfield_observer_present(other, events.field, &present) == FVM_INVALID_ARGUMENT && !present))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (modern)
            {
                FVM_Object *bare = 0;
                if (!(fvm_textfield_create(c, "Geometry", 100, 100, &bare) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *saved = bare->prototype;
                bare->prototype = foreign;
                present = true;
                if (!(fvm_textfield_observer_present(c, bare, &present) == FVM_INVALID_ARGUMENT && !present))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bare->prototype = saved;
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_context_destroy(other);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

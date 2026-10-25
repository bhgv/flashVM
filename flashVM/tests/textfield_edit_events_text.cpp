/* Counted text edits publish effective ranges and content before field/listener callbacks. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"

#ifdef FVM_HAS_FONTS
struct EditContent
{
    GeometryObserver observer;
    const char *text;
    size_t size;
};


static FVM_Result
edit_content(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    EditContent *data = (EditContent *)raw;
    counted_text(c, data->observer.events->field, data->text, data->size);
    return geometry_observe(c, self, n, args, out, &data->observer);
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
    GeometryEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    scroll_number(c, events.field, "scroll", 2);
    scroll_number(c, events.field, "hscroll", 7);
    EditContent own = {{&events, 1}, "A", 1}, listener = {{&events, 2}, "A", 1};
    FVM_Object *observer = 0;
    if (!(fvm_object_create(c, &observer) == FVM_OK &&
          fvm_object_define_method_rooted(c, events.field, "onScroller", edit_content, &own, events.field) == FVM_OK &&
          fvm_object_define_method_rooted(c, observer, "onScroller", edit_content, &listener, events.field) == FVM_OK &&
          scroll_listen(c, events.field, observer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(edit_text(c, events.field, "A", 1) == FVM_OK && events.count == 2 && events.last.scroll == 1 &&
          events.last.maxscroll == 1 && events.last.hscroll == 0 && events.last.maxhscroll == 0 &&
          events.order[0] == 1 && events.order[1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    own.text = listener.text = "VA\nVA\nVA";
    own.size = listener.size = 8;
    if (!(edit_text(c, events.field, own.text, own.size) == FVM_OK && events.count == 4 && events.last.maxscroll == 2 &&
          events.last.maxhscroll == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(edit_text(c, events.field, "AV\nAV\nAV", 8) == FVM_OK && events.count == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, events.field, "AV\nAV\nAV", 8);
    own.text = listener.text = "";
    own.size = listener.size = 0;
    if (!(edit_text(c, events.field, "", 0) == FVM_OK && events.count == 6 && events.last.maxscroll == 1 &&
          !events.last.maxhscroll))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(c, events.field, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(events.count == 6 && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
          !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}

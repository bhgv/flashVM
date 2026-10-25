/* Loaded dynamic handler resolution must execute once after publication, never during admission. */
#define FVM_SCROLL_EVENTS_TRANSFER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_transfer.cpp"

#ifdef FVM_HAS_FONTS
struct GeometryResolver
{
    FVM_Object *field;
    unsigned calls;
};


static FVM_Result
geometry_resolve(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    GeometryResolver *data = (GeometryResolver *)raw;
    if (!(self == data->field && n == 1 && args[0].type == FVM_VALUE_STRING &&
          !std::strcmp(args[0].string, "onScroller")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(counted_metric(c, self, "_width") == 60 && scroll_info(c, self).maxhscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++data->calls;
    return fvm_object_get(c, self, "handlerValue", out);
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
    for (unsigned version = 6; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char movie[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        movie[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, movie, sizeof(movie)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = scroll_field(c), *listener = script_listener(c);
        FVM_Value value;
        if (!(fvm_object_get(c, listener, "onScroller", &value) == FVM_OK &&
              fvm_object_set(c, field, "handlerValue", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 0);
        if (!(fvm_object_set(c, field, "seen", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        GeometryResolver data = {field, 0};
        if (!(fvm_object_define_method_rooted(c, field, "__resolve", geometry_resolve, &data, field) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scroll_number(c, field, "_width", 60);
        if (!(data.calls == 1 && counted_metric(c, field, "seen") == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scroll_number(c, field, "_width", 60);
        if (!(data.calls == 1 && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
              !c->avm1_execution_depth))
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

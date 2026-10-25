/* Imported multi-block calls retain captured strings through source mutation, actual GC and graph transfer. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"
#include "flashvm/extensions.h"


struct Mutation
{
    FVM_Object *record;
    unsigned calls;
};


static FVM_Result mutate(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Mutation *p = static_cast<Mutation *>(raw);
    ++p->calls;
    p->record->properties.erase("segments");
    if (!(fvm_value_string(&p->record->properties["code"], "00") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    (void)c;
    fvm_value_undefined(out);
    return FVM_OK;
}


static Bytes mutation_code()
{
    Bytes b;
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    u32(b, 0);
    text(b, "mutate");
    b.push_back(0x3d);
    b.push_back(0x17);
    text(b, "collected");
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    u32(b, 0);
    text(b, "gc");
    b.push_back(0x1c);
    text(b, "collect");
    b.push_back(0x52);
    b.push_back(0x1d);
    b.push_back(0);
    return b;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned route = 0; route < 4; ++route)
        {
            if (route == 3 && !fvm_jit_available())
            {
                continue;
            }
            FVM_Context *source = tick_context(rt, version), *c = tick_context(rt, version);
            Bytes tags, empty;
            tag(tags, 12, mutation_code());
            tag(tags, 12, action(99));
            tag(tags, 1, empty);
            tag(tags, 0, empty);
            Bytes swf = movie(tags, 1);
            swf[3] = (unsigned char)version;
            FVM_Object *root = 0;
            if (!(fvm_movieclip_load_swf(source, swf.begin(), swf.size(), "source", &root) == FVM_OK &&
                  fvm_gc_pin(source, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Channel *ch = 0;
            if (!(fvm_channel_create(&ch) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value input = tick_object(root), out;
            if (!(fvm_channel_send_value(source, ch, &input, 0) == FVM_OK &&
                  fvm_channel_receive_value(c, ch, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            root = out.object;
            if (!(fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(ch);
            fvm_context_destroy(source);
            if (!(fvm_extensions_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Mutation p = {root->resource_result.object->properties["frames"].object->indexed[1].object, 0};
            if (!(fvm_register_function_rooted(c, "mutate", mutate, &p, p.record) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->debug_function = "caller";
            c->debug_offset = 47;
            if (!(run_frame(c, root, route) == FVM_OK && p.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, root, "seen", 99);
            number(c, root, "_currentframe", 1);
            if (route < 2)
            {
                if (!(c->debug_function == "caller" && c->debug_offset == 47))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Value collected;
            if (!(fvm_object_get(c, root, "collected", &collected) == FVM_OK && collected.type == FVM_VALUE_NUMBER &&
                  collected.number >= 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            if (route == 3)
            {
                if (!(c->execution_mode.stats.native_entries > 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}

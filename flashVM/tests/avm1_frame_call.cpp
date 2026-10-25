/* Frame Call executes only actions, roots copied blocks and bounds recursive frame entry. */
#define main timeline_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_movieclip_timeline.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_state.hpp"
#include "flashvm/extensions.h"
#include "flashvm/avm1.h"


static Bytes invoke_number(unsigned long frame)
{
    Bytes b;
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    u32(b, frame);
    b.push_back(0x9e);
    u16(b, 0);
    b.push_back(0);
    return b;
}


static Bytes invoke_text(const char *label)
{
    Bytes b;
    text(b, label);
    b.push_back(0x9e);
    u16(b, 0);
    b.push_back(0);
    return b;
}


static void execute(FVM_Context *c, FVM_Object *clip, const Bytes &b)
{
    if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result detach(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *record = (FVM_Object *)raw;
    record->properties.erase("segments");
    fvm_value_string(&record->properties["code"], "00");
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_create(c, "root", 3, &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value root;
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = clip;
    if (!(fvm_set_global(c, "_root", &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body = action(2);
    if (!(fvm_movieclip_define_frame(c, clip, 2, "second", body.begin(), body.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    execute(c, clip, invoke_number(2));
    number(c, clip, "seen", 2);
    number(c, clip, "_currentframe", 1);
    body = action(3);
    if (!(fvm_movieclip_define_frame(c, clip, 3, "third", body.begin(), body.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    execute(c, clip, invoke_text("third"));
    number(c, clip, "seen", 3);
    number(c, clip, "_currentframe", 1);
    execute(c, clip, invoke_text("2"));
    number(c, clip, "seen", 2);
    execute(c, clip, invoke_number(2UL));
    number(c, clip, "seen", 2);
    FVM_Value wrapped;
    fvm_value_number(&wrapped, 4294967298.0);
    if (!(fvm_movieclip_call_frame(c, clip, wrapped) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(c, clip, "seen", 2);
    execute(c, clip, invoke_number(0));
    execute(c, clip, invoke_number(65536));
    execute(c, clip, invoke_text("missing"));
    number(c, clip, "seen", 2);
    Bytes call = invoke_number(3);
    if (!(fvm_avm1_execute(c, call.begin(), call.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(c, clip, "seen", 3);
    FVM_Value create[2], created;
    fvm_value_string(create, "child");
    fvm_value_number(create + 1, 10);
    if (!(fvm_object_call(c, clip, "createEmptyMovieClip", 2, create, &created) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *target = created.object;
    body = action(77);
    if (!(fvm_movieclip_define_frame(c, target, 1, "first", body.begin(), body.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    execute(c, clip, invoke_text("child:first"));
    number(c, target, "seen", 77);
    number(c, clip, "seen", 3);
    execute(c, clip, invoke_text("/child:1"));
    execute(c, target, invoke_text("..:third"));
    number(c, clip, "seen", 3);
    number(c, target, "_currentframe", 1);
    FVM_Value counted;
    fvm_value_string_bytes(&counted, "child\0suffix", 12);
    if (!(fvm_object_set(c, target, "_name", &counted) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string_bytes(&counted, "child\0suffix:1", 14);
    if (!(fvm_set_global(c, "input", &counted) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes counted_call;
    text(counted_call, "input");
    counted_call.push_back(0x1c);
    counted_call.push_back(0x9e);
    u16(counted_call, 0);
    counted_call.push_back(0);
    execute(c, clip, counted_call);
    number(c, target, "seen", 77);
    call = invoke_text("/root:second");
    if (!(fvm_avm1_execute_receiver(c, call.begin(), call.size(), clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char empty[] = {0x9e, 0, 0, 0}, invalid[] = {0x9e, 1, 0, 0, 0};
    if (!(fvm_avm1_execute_receiver(c, empty, sizeof(empty), clip) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_execute_receiver(c, invalid, sizeof(invalid), clip) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call = invoke_number(2);
    if (!(fvm_movieclip_define_frame(c, clip, 2, "second", call.begin(), call.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_execute_receiver(c, call.begin(), call.size(), clip) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->script_depth == 0 && c->avm1_execution_depth == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    body = action(2);
    if (!(fvm_movieclip_define_frame(c, clip, 2, "second", body.begin(), body.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    execute(c, clip, call);
    number(c, clip, "seen", 2);
    /* Import two separate DoAction blocks on frame two, detach their source during the first. */
    Bytes tags, blank, first;
    first.push_back(0x96);
    u16(first, 5);
    first.push_back(7);
    u32(first, 0);
    text(first, "detach");
    first.push_back(0x3d);
    first.push_back(0x17);
    text(first, "collected");
    first.push_back(0x96);
    u16(first, 5);
    first.push_back(7);
    u32(first, 0);
    text(first, "gc");
    first.push_back(0x1c);
    text(first, "collect");
    first.push_back(0x52);
    first.push_back(0x1d);
    first.push_back(0);
    tag(tags, 1, blank);
    tag(tags, 12, first);
    tag(tags, 12, action(99));
    tag(tags, 1, blank);
    tag(tags, 0, blank);
    Bytes swf = movie(tags, 2);
    FVM_Object *imported = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "imported", &imported) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    root.object = imported;
    if (!(fvm_set_global(c, "_root", &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *frames = imported->resource_result.object->properties["frames"].object;
    FVM_Object *record = frames->indexed[2].object;
    if (!(fvm_register_function_rooted(c, "detach", detach, record, record) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    execute(c, imported, invoke_number(2));
    number(c, imported, "seen", 99);
    FVM_Value collected;
    if (!(fvm_object_get(c, imported, "collected", &collected) == FVM_OK && collected.type == FVM_VALUE_NUMBER &&
          collected.number >= 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(c, imported, "_currentframe", 1);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}

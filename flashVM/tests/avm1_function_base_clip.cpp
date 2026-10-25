/* A closure's timeline is independent of its invocation receiver, including native bodies. */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_target_scope.cpp"
#include "flashvm/execution.h"


static FVM_Result native_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    if (!(c->avm1_base_clip == (FVM_Object *)raw && c->avm1_target_clip == c->avm1_base_clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result status = fvm_exec_frame_control(c, c->avm1_base_clip, FVM_EXEC_STOP, 0, 0);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = frame->receiver;
    return status;
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
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = 0;
        if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *definition = child(c, root, "definition", 1), *receiver = child(c, root, "receiver", 2);
        Bytes body;
        variable(body, "mark", 77);
        body.push_back(0x07);
        global(body, "this");
        body.push_back(0x3e);
        Bytes header;
        header.push_back(0);
        header.push_back(0);
        header.push_back(0);
        header.push_back((unsigned char)body.size());
        header.push_back(0);
        Bytes code;
        code.push_back(0x9b);
        code.push_back((unsigned char)header.size());
        code.push_back(0);
        for (size_t i = 0; i < header.size(); ++i)
        {
            code.push_back(header[i]);
        }
        for (size_t i = 0; i < body.size(); ++i)
        {
            code.push_back(body[i]);
        }
        code.push_back(0x3e);
        code.push_back(0);
        FVM_Object *factory = 0;
        if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &factory) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value out;
        if (!(fvm_function_invoke(c, factory, definition, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *closure = out.object;
        if (!(fvm_gc_pin(c, closure) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, closure, receiver, 0, 0, &out) == FVM_OK && out.object == receiver))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, definition, "mark", 77);
        private_number(definition, "playing", 0);
        private_number(receiver, "playing", 1);
        if (!(fvm_function_set_body(c, closure, native_body, definition, definition) == FVM_OK &&
              fvm_function_invoke(c, closure, receiver, 0, 0, &out) == FVM_OK && out.object == receiver))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Channel *channel = 0;
        if (!(fvm_channel_create(&channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out.object = closure;
        if (!(fvm_channel_send_value(c, channel, &out, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copied_base = copy->resource_result.object->properties["base_clip"].object;
        FVM_Object *new_receiver = 0;
        if (!(fvm_movieclip_create(other, "receiver", 1, &new_receiver) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(other, copy, new_receiver, 0, 0, &out) == FVM_OK && out.object == new_receiver))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(other, copied_base, "mark", 77);
        private_number(copied_base, "playing", 0);
        if (!(!c->avm1_target_active && !other->avm1_target_active && !c->gc_frames && !other->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(channel);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

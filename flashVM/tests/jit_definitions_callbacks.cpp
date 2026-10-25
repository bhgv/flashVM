/* Named native definitions survive collecting/throwing script setters on the selected timeline. */
#define FVM_DEFINITIONS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_definitions.cpp"
#include "flashvm/movieclip.h"
#include "../src/avm1/block.hpp"

static unsigned setter_calls;
static bool setter_throws;


static FVM_Result definition_setter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++setter_calls;
    if (!(frame->argc == 1 && frame->argv[0].type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value result;
    if (!(fvm_function_invoke(c, frame->argv[0].object, 0, 0, 0, &result) == FVM_OK &&
          result.type == FVM_VALUE_STRING && fvm_value_string_size(&result) == 8 &&
          !std::memcmp(result.string, "snapshot", 8)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "saved", frame->argv) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 17);
    return setter_throws ? fvm_exception_raise(c, out) : FVM_OK;
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned second = 0; second < 2; ++second)
        {
            for (unsigned native = 0; native < 2; ++native)
            {
                for (unsigned throwing = 0; throwing < 2; ++throwing)
                {
                    if (version && version < 4)
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (version)
                    {
                        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *root = 0, *setter = 0, *getter = 0;
                    const unsigned char empty[] = {0};
                    FVM_Value args[3], out;
                    if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK &&
                          fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &setter) == FVM_OK &&
                          fvm_function_set_body(c, setter, definition_setter, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value constructor, prototype;
                    if (!(fvm_get_global(c, "Object", &constructor) == FVM_OK &&
                          fvm_object_get(c, constructor.object, "prototype", &prototype) == FVM_OK &&
                          fvm_object_set_prototype(c, root, prototype.object) == FVM_OK &&
                          fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &getter) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_string(args, "made");
                    args[1] = class_value(getter);
                    args[2] = class_value(setter);
                    if (!(fvm_object_call(c, root, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program body;
                    body.op(0x96);
                    definition_word(body, 2);
                    body.op(8);
                    body.op(0);
                    body.finish();
                    Program p;
                    definition_pool(p, "snapshot");
                    definition_record(p, "made", second, false, body);
                    p.op(0);
                    if (!(fvm_execution_set_mode(c, native ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    setter_calls = 0;
                    setter_throws = throwing != 0;
                    FVM_GCRootFrame *roots = c->gc_frames;
                    if (!(fvm_avm1_execute_block_receiver(c, p.bytes.begin(), p.bytes.size(), root) ==
                          (throwing ? FVM_EXCEPTION : FVM_OK)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(setter_calls == 1 && c->gc_frames == roots && !c->script_depth && !c->gc_native_depth &&
                          !c->avm1_execution_depth && !c->execution_mode.native_pool))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (throwing)
                    {
                        if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 17 &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_get_global(c, "saved", &out) == FVM_OK &&
                          out.type == FVM_VALUE_OBJECT))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    class_mode = native ? 2 : 0;
                    FVM_Value result = definition_invoke(c, out.object, body);
                    if (!(result.type == FVM_VALUE_STRING && !std::strcmp(result.string, "snapshot")))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}

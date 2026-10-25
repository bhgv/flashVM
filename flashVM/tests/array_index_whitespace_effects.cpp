/* Whitespace classification precedes own setters and preserves literal-key storage. */
#define FVM_ARRAY_INDEX_WHITESPACE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_index_whitespace.cpp"

static unsigned whitespace_calls, whitespace_failure;
static double whitespace_expected;
static FVM_Object *whitespace_target;


static FVM_Result whitespace_setter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++whitespace_calls;
    if (!(frame->receiver == whitespace_target))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value length;
    if (!(fvm_object_get(c, whitespace_target, "length", &length) == FVM_OK && length.number == whitespace_expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, c->global, "whitespaceTarget", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, whitespace_target, "length", &length) == FVM_OK && length.number == whitespace_expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 77);
    return whitespace_failure == 1 ? fvm_exception_raise(c, out) : whitespace_failure == 2 ? FVM_ERROR : FVM_OK;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned vertical = 0; vertical < 2; ++vertical)
        {
            for (unsigned inherited = 0; inherited < 2; ++inherited)
            {
                for (unsigned fixed = 0; fixed < 2; ++fixed)
                {
                    for (whitespace_failure = 0; whitespace_failure < 3; ++whitespace_failure)
                    {
                        for (unsigned route = 0; route < 4; ++route)
                        {
                            if (route >= 2 && !fvm_jit_available())
                            {
                                continue;
                            }
                            FVM_Context *c = 0;
                            if (!(fvm_context_create(rt, &c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            version_load(c, version);
                            if (!(fvm_standard_install(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value initial[3], value, args[3], out;
                            for (unsigned i = 0; i < 3; ++i)
                            {
                                fvm_value_number(initial + i, i + 10);
                            }
                            if (!(fvm_construct(c, "Array", 3, initial, &whitespace_target) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(whitespace_target);
                            if (!(fvm_set_global(c, "whitespaceTarget", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_object_set_property_flags(c, whitespace_target, "1", 2, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *owner = whitespace_target, *setter = version_function(c, 0);
                            if (inherited)
                            {
                                if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK &&
                                      fvm_object_set_prototype(c, whitespace_target, owner) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(fvm_function_set_body(c, setter, whitespace_setter, 0, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            const char *key = vertical ? "\v+2147483647" : "\f+2147483647";
                            fvm_value_string(args, key);
                            args[1] = class_value(version_function(c, 42));
                            args[2] = class_value(setter);
                            if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (fixed)
                            {
                                if (!(fvm_object_set_property_flags(c, whitespace_target, "length", 4, 0) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            whitespace_expected = !vertical && !inherited && !fixed ? -2147483648.0 : 3;
                            whitespace_calls = 0;
                            const FVM_Result expected = whitespace_failure == 1   ? FVM_EXCEPTION
                                                        : whitespace_failure == 2 ? FVM_ERROR
                                                                                  : FVM_OK;
                            if (!route)
                            {
                                fvm_value_number(&value, 81);
                                if (!(fvm_object_set(c, whitespace_target, key, &value) == expected))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (expected == FVM_EXCEPTION)
                                {
                                    if (!(fvm_exception_clear(c) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            else
                            {
                                Program body;
                                body.variable("whitespaceTarget");
                                body.text(key);
                                body.number(81);
                                body.op(0x4f);
                                body.number(1);
                                body.finish();
                                out = boxing_run(c, body, route - 1, expected);
                            }
                            if (!(whitespace_calls == 1 && !c->exception_pending))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            /* The setter detached the global; pin the returned-to host pointer before GC. */
                            if (!(fvm_gc_pin(c, whitespace_target) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            for (unsigned i = 0; i < 3; ++i)
                            {
                                char index[2] = {(char)('0' + i), 0};
                                int own = 0;
                                if (!(fvm_object_has_own(c, whitespace_target, index, &own) == FVM_OK &&
                                      !!own == (vertical || inherited || i == 1)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(fvm_object_get(c, whitespace_target, "length", &out) == FVM_OK &&
                                  out.number == whitespace_expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner &&
                                  !c->script_this_cell))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_context_destroy(c);
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}

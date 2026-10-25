/* The public execution service includes native operand coercion bytecode in its outer frame-call limit. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"


static FVM_Result convert(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *clip = static_cast<FVM_Object *>(raw);
    Bytes code = action(111);
    FVM_Result status = fvm_avm1_execute_receiver(c, code.begin(), code.size(), clip);
    if (status != FVM_OK)
    {
        return status;
    }
    return fvm_value_string(out, "1");
}


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned long limit = 9; limit <= 14; limit += 5)
    {
        FVM_Runtime *rt = 0;
        cfg.instruction_limit = limit;
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *c = tick_context(rt);
        FVM_Object *clip = tick_root(c), *input = 0;
        Bytes code = action(222);
        if (!(fvm_movieclip_define_frame(c, clip, 1, 0, code.begin(), code.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &input) == FVM_OK && fvm_gc_pin(c, input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, input, "toString", convert, clip, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value argument = tick_object(input);
        if (!(fvm_exec_call_frame(c, clip, &argument) == (limit == 9 ? FVM_ERROR : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, clip, "seen", limit == 9 ? 111 : 222);
        tick_clean(c);
        if (!(c->avm1_steps == (limit == 9 ? 10UL : 14UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
}

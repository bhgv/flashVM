/* Direct/value frame checks retain distinct limits, strict decimal syntax and Int32 wrapping. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include <math.h>


static void check(FVM_Context *c, FVM_ExecutionFrameCheck policy, FVM_Value *input, int expected)
{
    if (!(fvm_exec_frame_loaded(c, policy, input, input) == FVM_OK && input->type == FVM_VALUE_BOOLEAN &&
          input->boolean == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Value input, out;
        unsigned i;
        const char *valid[] = {"16002", "+16002.", "1.6002e4", "16002.0", "-2147483648"};
        const char *labels[] = {"label", " 16002", "16002 ", "0x3e82", "16002e", "16002.5", "Infinity"};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 16000);
        check(c, FVM_EXEC_FRAME_CHECK_DIRECT, &input, 1);
        fvm_value_number(&input, 16001);
        check(c, FVM_EXEC_FRAME_CHECK_DIRECT, &input, 0);
        fvm_value_number(&input, 16001);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        fvm_value_number(&input, 16002);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 0);
        fvm_value_number(&input, -2147483648.0);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 0);
        fvm_value_number(&input, 4294983298.0);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 0);
        fvm_value_number(&input, -1);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        fvm_value_number(&input, 16002.5);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        fvm_value_number(&input, HUGE_VAL);
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        for (i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i)
        {
            if (!(fvm_value_string(&input, valid[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 0);
        }
        for (i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i)
        {
            if (!(fvm_value_string(&input, labels[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        }
        if (!(fvm_value_string_bytes(&input, "16002\0x", 7) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check(c, FVM_EXEC_FRAME_CHECK_VALUE, &input, 1);
        fvm_value_number(&out, 99);
        fvm_value_number(&input, -1);
        if (!(fvm_exec_frame_loaded(c, FVM_EXEC_FRAME_CHECK_DIRECT, &input, &out) == FVM_INVALID_ARGUMENT &&
              out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_loaded(c, (FVM_ExecutionFrameCheck)99, &input, &out) == FVM_INVALID_ARGUMENT &&
              out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_loaded(0, FVM_EXEC_FRAME_CHECK_VALUE, &input, &out) == FVM_INVALID_ARGUMENT &&
              out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

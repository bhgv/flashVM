/* C89 frame preparation preserves aliases, counted labels, numeric bias and zero signs. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include <string.h>


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned mode;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (mode = 0; mode < 6; ++mode)
    {
        FVM_Context *c;
        FVM_Value in, out;
        unsigned i;
        const char *labels[] = {"", "second", "+2", "-1", "2.5", " 2", "2e1"};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)(mode + 3);
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&in, 2);
        if (!(fvm_exec_frame_request(c, &in, 1, 65535, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
              out.number == 65537))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&in, "002") == FVM_OK && fvm_exec_frame_request(c, &in, 1, 3, &in) == FVM_OK &&
              in.type == FVM_VALUE_NUMBER && in.number == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < sizeof(labels) / sizeof(labels[0]); ++i)
        {
            if (!(fvm_value_string(&in, labels[i]) == FVM_OK && fvm_exec_frame_request(c, &in, 1, 7, &out) == FVM_OK &&
                  out.type == FVM_VALUE_STRING && strcmp(out.string, labels[i]) == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_value_string_bytes(&in, "2\0abc", 5) == FVM_OK &&
              fvm_exec_frame_request(c, &in, 1, 7, &in) == FVM_OK && in.type == FVM_VALUE_STRING &&
              fvm_value_string_size(&in) == 5 && memcmp(in.string, "2\0abc", 5) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&in);
        if (!(fvm_exec_frame_request(c, &in, 1, 3, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
              strcmp(out.string, mode && mode + 3 < 7 ? "" : "undefined") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&in, 1);
        if (!(fvm_exec_frame_request(c, &in, 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode == 1)
        {
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.type == FVM_VALUE_STRING && strcmp(out.string, "true") == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&in, -0.0);
        if (!(fvm_exec_frame_request(c, &in, 0, 0, &out) == FVM_OK && out.number == 0 && 1.0 / out.number < 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_request(c, &in, 1, 0, &out) == FVM_OK && out.number == 0 && 1.0 / out.number > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_frame_request(0, &in, 0, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_request(c, &in, 1, 65536, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_request(c, 0, 0, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_frame_request(c, &in, 0, 0, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

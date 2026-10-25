/* With still executes on a bare object when a primitive's global class is unavailable. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"


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
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body;
        text(body, "probe");
        global(body, "valueOf");
        body.push_back(0x1d);
        text(body, "ran");
        integer(body, 17);
        body.push_back(0x1d);
        Bytes code;
        integer(code, 42);
        code.push_back(0x94);
        code.push_back(2);
        code.push_back(0);
        code.push_back((unsigned char)body.size());
        code.push_back((unsigned char)(body.size() >> 8));
        for (size_t i = 0; i < body.size(); ++i)
        {
            code.push_back(body[i]);
        }
        text(code, "done");
        run(c, code);
        if (!(fvm_get_global(c, "probe", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED &&
              fvm_get_global(c, "ran", &value) == FVM_OK && value.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

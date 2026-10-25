/* ASSetPropFlags preserves target flags at every failed boxing/conversion/map allocation. */
#define main native_instance_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "property_flags_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; version += 2)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            bool failed = false, success = false;
            for (long boundary = 0; boundary < 1500 && !success; ++boundary)
            {
                FlagFixture f;
                flag_init(rt, f, version, type);
                f.mutate = false;
                FVM_Context *c = f.context;
                FVM_Value out;
                fvm_value_number(&out, 91);
                allowance = boundary;
                FVM_Result status = fvm_call(c, "ASSetPropFlags", 4, f.args, &out);
                allowance = -1;
                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_constructor &&
                      out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OUT_OF_MEMORY)
                {
                    failed = true;
                    flag_check(c, f.target, 0);
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    f.used = 0;
                    if (!(fvm_call(c, "ASSetPropFlags", 4, f.args, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                if (!(f.used == 4 && !std::memcmp(f.trace, "TSCN", 4)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                flag_check(c, f.target, 3);
                fvm_context_destroy(c);
            }
            if (!(failed && success))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_runtime_destroy(rt);
}

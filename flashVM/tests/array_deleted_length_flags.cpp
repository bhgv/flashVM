/* Ordinary recreated numeric length preserves readonly flags while native tail deletion still runs. */
#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_deleted_length.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *methods[] = {"pop", "shift", "push", "unshift"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned method = 0; method < 4; ++method)
        {
            for (unsigned readonly = 0; readonly < 2; ++readonly)
            {
                for (unsigned protected_index = 0; protected_index < 2; ++protected_index)
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
                        FVM_Value raw, out;
                        unsigned count = 0;
                        FVM_Object *array = array_deleted_fixture(c, 3, raw, count);
                        if (readonly)
                        {
                            if (!(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (protected_index)
                        {
                            if (!(fvm_object_set_property_flags(c, array, "0", 2, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        out = array_deleted_call(c, array, methods[method], route);
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == (method < 2 ? 11 : 2)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_get(c, array, "length", &raw) == FVM_OK && raw.type == FVM_VALUE_NUMBER &&
                              raw.number == (readonly     ? 1
                                             : method < 2 ? 0
                                                          : 2)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, "0", &own) == FVM_OK &&
                              !!own == (method >= 2 || protected_index)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_table_get_index(c, array, 1, &raw) == FVM_OK && raw.number == (method == 2   ? 81
                                                                                                 : method == 3 ? 11
                                                                                                               : 22)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        unsigned flags = 0;
                        if (!(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK &&
                              flags == (readonly ? 4U : 0U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth))
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
    fvm_runtime_destroy(rt);
}

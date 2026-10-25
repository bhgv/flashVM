/* Native constructor registrations share loaded case selection and replacement ownership. */
#define main replacement_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "global_native_replacement.cpp"

#undef main


static FVM_Result case_factory(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *userdata)
{
    *out = replacement_value((FVM_Object *)userdata);
    return FVM_OK;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *stored[] = {"NativeMix", "\xc3\x84\0Key"}, *queried[] = {"nATIVEmIX", "\xc3\xa4\0kEY"};
    const size_t sizes[] = {9, 6};
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned which = 0; which < 2; ++which)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            if (!(fvm_context_create(runtime, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
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
            FVM_Object *old_capture = replacement_capture(c, 1), *result = 0;
            size_t count = c->constructors.size();
            if (!(fvm_register_constructor_rooted_bytes(
                      c, stored[which], sizes[which], case_factory, old_capture, old_capture) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool folded = version && version <= 6;
            if (!(fvm_construct_bytes(c, queried[which], sizes[which], 0, 0, &result) ==
                      (folded ? FVM_OK : FVM_UNSUPPORTED) &&
                  result == (folded ? old_capture : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *new_capture = replacement_capture(c, 2);
            if (!(fvm_register_constructor_rooted_bytes(
                      c, queried[which], sizes[which], case_factory, new_capture, new_capture) == FVM_OK &&
                  fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->constructors.size() == count + (folded ? 1U : 2U) &&
                  replacement_alive(c, old_capture) == !folded && replacement_alive(c, new_capture)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_construct_bytes(c, stored[which], sizes[which], 0, 0, &result) == FVM_OK &&
                  result == (folded ? new_capture : old_capture)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_construct_bytes(c, queried[which], sizes[which], 0, 0, &result) == FVM_OK &&
                  result == new_capture))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}

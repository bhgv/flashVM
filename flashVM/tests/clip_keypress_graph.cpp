/* Exact KeyPress selectors survive graph transfer, source teardown, traced strings and native JIT. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_keypress_fixture.hpp"


/* Source-version gates travel with the retained resource library rather than sender lifetime. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned jit = 0; jit < 2; ++jit)
        {
            if (jit && !fvm_jit_available())
            {
                continue;
            }
            FVM_Context *source = clip_context(runtime), *dest = clip_context(runtime);
            FVM_Object *root = keypress_root(source, version, 'A');
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = root;
            unsigned char *wire = 0;
            size_t size = 0;
            if (!(fvm_value_serialize(source, &value, &wire, &size) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(source);
            if (!(fvm_value_deserialize(dest, wire, size, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(wire);
            root = value.object;
            if (!(fvm_gc_pin(dest, root) == FVM_OK && fvm_gc_string_mode(dest, FVM_GC_STRINGS_TRACED) == FVM_OK &&
                  fvm_gc_collect(dest, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(dest, jit ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned long before = dest->execution_mode.stats.native_entries;
            if (!(fvm_movieclip_dispatch_key_press(dest, root, 'a') == FVM_OK &&
                  dest->execution_mode.stats.native_entries == before))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_dispatch_key_press(dest, root, 'A') == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!((dest->execution_mode.stats.native_entries > before) == (jit != 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(dest, get(dest, root, "actor").object, "seen", 42);
            input_clean(dest);
            fvm_context_destroy(dest);
        }
    }
    fvm_runtime_destroy(runtime);
}

/* Exercise the shipped interactive SWF through repeated loops and short taps in both VM modes. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
#include "flashvm/gc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


static FVM_Result ignore_sound(FVM_Context *, FVM_Object *, size_t, void *)
{
    return FVM_OK;
}


int main(int argc, char **argv)
{
    if (!(argc == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::FILE *file = std::fopen(argv[1], "rb");
    if (!(file))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fseek(file, 0, SEEK_END) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long length = std::ftell(file);
    if (!(length > 0 && std::fseek(file, 0, SEEK_SET) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char *bytes = (unsigned char *)std::malloc((size_t)length);
    if (!(bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fread(bytes, 1, (size_t)length, file) == (size_t)length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::fclose(file);
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < (fvm_jit_available() ? 2U : 1U); ++mode)
    {
        FVM_Context *context = 0;
        FVM_Object *clip = 0, *host = 0;
        if (!(fvm_context_create(runtime, &context) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_load_memory(context, bytes, (size_t)length) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_selection_install(context) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_load_swf(context, bytes, (size_t)length, "_root", &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(context, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(context, &host) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = host;
        if (!(fvm_set_global(context, "HostInput", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 0);
        const char *fields[] = {"x", "y", "buttons", "clicks"};
        for (unsigned i = 0; i < 4; ++i)
        {
            if (!(fvm_object_set(context, host, fields[i], &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_set_sound_callback(context, ignore_sound, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_execution_set_mode(context, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value key, listeners, size;
        if (!(fvm_get_global(context, "Key", &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned cycle = 1; cycle <= 3; ++cycle)
        {
            FVM_KeyboardState state;
            std::memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            state.last_code = FVM_KEY_LEFT;
            state.down[FVM_KEY_LEFT] = 1;
            if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            state.down[FVM_KEY_LEFT] = 0;
            if (!(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_advance(context, clip, 120) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value observer;
            if (!(fvm_object_get(context, clip, "KeyObserver", &observer) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, observer.object, "downs", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(value.number == cycle))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, observer.object, "ups", &value) == FVM_OK && value.number == cycle))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, key.object, "_listeners", &listeners) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, listeners.object, "length", &size) == FVM_OK && size.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, clip, "FocusDowns", &value) == FVM_OK && value.number == cycle))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(context, clip, "FocusUps", &value) == FVM_OK && value.number == cycle))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *focused = 0;
            if (!(fvm_input_get_keyboard_focus(context, &focused) == FVM_OK && focused == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(context, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(context);
    }
    fvm_runtime_destroy(runtime);
    std::free(bytes);
}

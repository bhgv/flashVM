/* Exercise the shipped interactive SWF through repeated loops and short taps in both VM modes. */
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static FVM_Result ignore_sound(FVM_Context *, FVM_Object *, size_t, void *) { return FVM_OK; }
int main(int argc, char **argv)
{
    assert(argc == 2);
    std::FILE *file = std::fopen(argv[1], "rb"); assert(file);
    assert(std::fseek(file, 0, SEEK_END) == 0);
    long length = std::ftell(file); assert(length > 0 && std::fseek(file, 0, SEEK_SET) == 0);
    unsigned char *bytes = (unsigned char *)std::malloc((size_t)length); assert(bytes);
    assert(std::fread(bytes, 1, (size_t)length, file) == (size_t)length); std::fclose(file);
    FVM_Config config; FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned mode = 0; mode < (fvm_jit_available() ? 2U : 1U); ++mode)
    {
        FVM_Context *context = 0; FVM_Object *clip = 0, *host = 0;
        assert(fvm_context_create(runtime, &context) == FVM_OK);
        assert(fvm_load_memory(context, bytes, (size_t)length) == FVM_OK);
        assert(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK);
        assert(fvm_movieclip_load_swf(context, bytes, (size_t)length, "_root", &clip) == FVM_OK);
        assert(fvm_gc_pin(context, clip) == FVM_OK);
        assert(fvm_object_create(context, &host) == FVM_OK);
        FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = host;
        assert(fvm_set_global(context, "HostInput", &value) == FVM_OK);
        fvm_value_number(&value, 0);
        const char *fields[] = {"x", "y", "buttons", "clicks"};
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set(context, host, fields[i], &value) == FVM_OK);
        assert(fvm_movieclip_set_sound_callback(context, ignore_sound, 0) == FVM_OK);
        assert(fvm_execution_set_mode(context, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK);
        assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK);
        FVM_Value key, listeners, size;
        assert(fvm_get_global(context, "Key", &key) == FVM_OK);
        for (unsigned cycle = 1; cycle <= 3; ++cycle)
        {
            FVM_KeyboardState state; std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
            state.last_code = FVM_KEY_LEFT; state.down[FVM_KEY_LEFT] = 1;
            assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
            state.down[FVM_KEY_LEFT] = 0;
            assert(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK);
            assert(fvm_movieclip_advance(context, clip, 120) == FVM_OK);
            FVM_Value observer;
            assert(fvm_object_get(context, clip, "KeyObserver", &observer) == FVM_OK);
            assert(fvm_object_get(context, observer.object, "downs", &value) == FVM_OK);
            assert(value.number == cycle);
            assert(fvm_object_get(context, observer.object, "ups", &value) == FVM_OK && value.number == cycle);
            assert(fvm_object_get(context, key.object, "_listeners", &listeners) == FVM_OK);
            assert(fvm_object_get(context, listeners.object, "length", &size) == FVM_OK && size.number == 1);
            assert(fvm_gc_collect(context, 0) == FVM_OK);
        }
        fvm_context_destroy(context);
    }
    fvm_runtime_destroy(runtime); std::free(bytes);
}

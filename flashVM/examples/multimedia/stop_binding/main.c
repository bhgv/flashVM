/* C89 tutorial: bind a host-prepared native voice to context StopSounds without fake PCM writes. */
#include "flashvm/multimedia.h" /* Import stop controls and context ownership APIs. */
#include "flashvm/standard.h" /* Install the real stopAllSounds script global. */
#include "flashvm/value.h" /* Invoke that global through the ordinary C ABI. */
#include <stdio.h> /* Report ordinary failures and the observable control result. */
#include <string.h> /* Require explicit demo selection before acquiring a VM. */
/* Keep host-owned prepared state separate from context-owned registration. */
typedef struct HostAudio /* This model demonstrates cancellation, not physical audio output. */
{ /* Begin native callback state. */
    int pending; /* Represent one prepared native voice not yet submitted as PCM. */
    unsigned int stops; /* Count explicit idempotent stop notifications. */
} HostAudio; /* Keep this stack state alive until binding close/context destruction. */
/* A real bridge would stop/clear its mixer, pending block and native FIFO here. */
static FVM_Result stop_prepared(void *raw) /* Borrow host state only for this synchronous callback. */
{ /* Begin cancellation independent of PCM channel activity. */
    HostAudio *audio = (HostAudio *)raw; /* Recover explicitly owned native state. */
    audio->pending = 0; /* Cancel even before any write ever occurred. */
    ++audio->stops; /* Record that the actual context control reached the host. */
    return FVM_OK; /* A real stop error would propagate to script and remain retryable. */
} /* Finish the allocation-free idempotent stop operation. */
/* Keep errors visible without assertions in tutorial code. */
static int checked(FVM_Result result, const char *name) /* Borrow one diagnostic label. */
{ /* Begin ordinary result handling. */
    if (result == FVM_OK) return 1; /* Continue only after acknowledged success. */
    fprintf(stderr, "%s: %d\n", name, (int)result); /* Preserve the original status. */
    return 0; /* Select reverse-order cleanup. */
} /* Finish result handling. */
/* Run only on explicit request; help performs no context/device initialization. */
int main(int argc, char **argv) /* Preserve strict C89 host compatibility. */
{ /* Begin separate runtime/context/registration/userdata ownership. */
    FVM_Config config; /* Receive the supported context/runtime configuration. */
    FVM_Runtime *runtime = 0; /* Own until the final cleanup step. */
    FVM_Context *context = 0; /* Own the stop registration unless explicitly detached. */
    FVM_AudioStopBinding *binding = 0; /* Borrow the context-owned handle until close. */
    FVM_Value result; /* Receive the ordinary script call result. */
    HostAudio audio; /* Keep native userdata alive through context teardown on every error path. */
    int status = 1; /* Report success only after every required operation. */
    audio.pending = 1; audio.stops = 0; /* Model a prepared voice before its first PCM write. */
    if (argc != 2 || strcmp(argv[1], "--demo")) /* Validate arguments before resource acquisition. */
    { /* Describe the explicit runnable mode. */
        fprintf(stderr, "usage: %s --demo\n", argv[0]); /* Never open an audio device in this control tutorial. */
        return argc == 1 ? 0 : 2; /* Help succeeds; malformed invocation fails. */
    } /* Finish command parsing. */
    if (!checked(fvm_config_init(&config), "config")) goto cleanup; /* Initialize before constructing the runtime. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) goto cleanup; /* Acquire native runtime ownership. */
    if (!checked(fvm_context_create(runtime, &context), "context")) goto cleanup; /* Acquire one isolated VM context. */
    if (!checked(fvm_standard_install(context), "standard objects")) goto cleanup; /* Install the real script control. */
    if (!checked(fvm_audio_stop_binding_create(context, stop_prepared, &audio, &binding), "binding")) goto cleanup; /* Copy callback identity without calling it. */
    if (!checked(fvm_call_bytes(context, "stopAllSounds", 13, 0, 0, &result), "stopAllSounds")) goto cleanup; /* Exercise actual script-to-native stopping. */
    printf("pending=%d stops=%u\n", audio.pending, audio.stops); /* Expect pending=0 stops=1 without any PCM write. */
    if (!checked(fvm_audio_stop_binding_close(binding), "detach")) goto cleanup; /* Detach without a second stop callback. */
    binding = 0; status = 0; /* Record released registration ownership and completed execution. */
cleanup: /* Keep native userdata live through any still-bound context teardown. */
    if (binding) fvm_audio_stop_binding_close(binding); /* Detach before host resources could disappear. */
    if (context) fvm_context_destroy(context); /* Invalidate every remaining context-owned handle. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release the outer native owner last. */
    return status; /* No managed pointer, callback or PCM buffer escapes the tutorial. */
} /* Finish the portable control example. */

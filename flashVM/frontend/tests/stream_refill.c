/* Compile the production C89 worker step directly; no test-only production API is exported. */
#include "../common/audio.c"
/* Tests supply exclusive serial ownership instead of starting a scheduling-dependent thread. */
FVM_Result fvm_test_stream_refill(FVM_DesktopAudio *audio)
{
    return refill_stream(audio);
}

/* Observe public validation calls while retaining the production owner-side audio implementation. */
#define fvm_swf_sound_read_pcm counted_pcm_read
#include "../common/audio.c"
#undef fvm_swf_sound_read_pcm
FVM_Result fvm_swf_sound_read_pcm(FVM_Context *, unsigned int, unsigned long, size_t, short *, size_t, size_t *);
static unsigned int calls = 0;
/* Reset only the test observation, never any source or production admission state. */
void fvm_test_preflight_reset(void) { calls = 0; }
/* Read the single-owner admission count after a complete synchronous validation. */
unsigned int fvm_test_preflight_calls(void) { return calls; }
/* Forward the real public read; a zero-length request must validate actual protected packets. */
FVM_Result counted_pcm_read(FVM_Context *c, unsigned int id, unsigned long first, size_t frames,
                            short *out, size_t capacity, size_t *written)
{
    ++calls;
    return fvm_swf_sound_read_pcm(c,id,first,frames,out,capacity,written);
}

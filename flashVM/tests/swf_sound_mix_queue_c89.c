/* C89 additive queue header/link and output preservation without a VM or SDL. */
#include "flashvm/swf_sound_mix_queue.h"
#include <string.h>
int main(void)
{
    FVM_SWFSoundMixQueue *queue = (FVM_SWFSoundMixQueue *)1;
    FVM_AudioQueueBackend backend, saved;
    FVM_SWFSoundMixQueueInfo info;
    memset(&backend, 0x5a, sizeof(backend));
    saved = backend;
    info.struct_size = sizeof(info);
    info.pending_frames = 91;
    if (fvm_swf_sound_mix_queue_create(0, 0, 1, &queue, &backend) != FVM_INVALID_ARGUMENT ||
        queue || memcmp(&backend, &saved, sizeof(saved)))
        return 1;
    if (fvm_swf_sound_mix_queue_info(0, &info) != FVM_INVALID_ARGUMENT || info.pending_frames != 91)
        return 1;
    fvm_swf_sound_mix_queue_close(0);
    return 0;
}

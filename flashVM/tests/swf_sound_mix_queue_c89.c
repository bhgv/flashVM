/* C89 additive queue header/link and output preservation without a VM or SDL. */
#include "flashvm/swf_sound_mix_queue.h"
#include <string.h>


int main(void)
{
    FVM_SWFSoundMixQueue *queue = (FVM_SWFSoundMixQueue *)1;
    FVM_AudioQueueBackend backend, saved;
    FVM_SWFSoundMixQueueInfo info;
    FVM_SWFSoundMixQueuePending pending;
    FVM_SWFSoundMixQueueStorageInfo storage;
    size_t accepted = 91, bytes = 91;
    memset(&backend, 0x5a, sizeof(backend));
    saved = backend;
    info.struct_size = sizeof(info);
    info.pending_frames = 91;
    if (fvm_swf_sound_mix_queue_create(0, 0, 1, &queue, &backend) != FVM_INVALID_ARGUMENT || queue ||
        memcmp(&backend, &saved, sizeof(saved)))
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_info(0, &info) != FVM_INVALID_ARGUMENT || info.pending_frames != 91)
    {
        return 1;
    }
    pending.struct_size = sizeof(pending);
    pending.frames = 91;
    pending.origin = 91;
    storage.struct_size = sizeof(storage);
    storage.retained_bytes = 91;
    if (fvm_swf_sound_mix_queue_pump_tail(0, 1, 1, &accepted) != FVM_INVALID_ARGUMENT || accepted)
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_pending(0, &pending) != FVM_INVALID_ARGUMENT || pending.frames != 91 ||
        pending.origin != 91)
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_storage_info(0, &storage) != FVM_INVALID_ARGUMENT || storage.retained_bytes != 91)
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_storage_required(0, 1, &bytes) != FVM_INVALID_ARGUMENT || bytes != 91)
    {
        return 1;
    }
    if (fvm_swf_sound_mix_queue_create_bounded(0, 0, 1, 0, &queue, &backend) != FVM_INVALID_ARGUMENT || queue ||
        memcmp(&backend, &saved, sizeof(saved)))
    {
        return 1;
    }
    fvm_swf_sound_mix_queue_close(0);
    return 0;
}

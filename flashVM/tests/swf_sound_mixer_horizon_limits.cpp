/* White-box repeat clamping proves bounded arithmetic without allocating an enormous source. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_mixer.cpp"


int main()
{
    FVM_SWFSoundVoiceInfo info;
    std::memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.frames_per_play = (size_t)-1;
    info.frame_in_play = (size_t)-2;
    info.play_count = UINT_MAX;
    if (!(source_horizon(info, 65536) == 65536))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.play_count = 1;
    if (!(source_horizon(info, 65536) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.frames_per_play = 1;
    info.frame_in_play = 0;
    info.play_count = UINT_MAX;
    if (!(source_horizon(info, 65536) == 65536))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.play_index = UINT_MAX - 2;
    if (!(source_horizon(info, 65536) == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.finished = 1;
    if (!(!source_horizon(info, 65536)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 91;
    if (!(fvm_swf_sound_mixer_active_frames(0, 1, &n) == FVM_INVALID_ARGUMENT && n == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}

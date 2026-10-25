/* Public C89 DefineSound lookup and signed PCM range decoding. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include <string.h>


int main(void)
{
    const unsigned char swf[] = {'F', 'W', 'S',  4, 31, 0, 0, 0, 8, 0,   0,   12,   1, 0, 0x8b, 3,
                                 7,   0,   0x3e, 2, 0,  0, 0, 1, 0, 254, 255, 0x40, 0, 0, 0};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_SWFSoundInfo info;
    FVM_SWFSoundVoice *voice = 0;
    FVM_SWFSoundVoiceInfo voice_info;
    FVM_SWFSoundEvent event;
    FVM_SWFSoundEnvelope point;
    short pcm[2] = {77, 88};
    size_t written = 99, sounds = 99;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&info, 0, sizeof(info));
    info.struct_size = sizeof(info);
    info.sound_id = 99;
    if (!(fvm_swf_sound_event_submit(c, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_info(c, 7, &info) == FVM_UNSUPPORTED && info.sound_id == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_UNSUPPORTED && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    voice_info.struct_size = sizeof(voice_info);
    if (!(fvm_swf_sound_voice_info(voice, &voice_info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_read(voice, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT && !written))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_close(voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_NOT_FOUND && !voice))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.struct_size = sizeof(event);
    point.struct_size = sizeof(point);
    if (!(fvm_swf_sound_event_count(c, &sounds) == FVM_OK && !sounds))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_at(c, 0, &event) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_event_envelope(c, 0, 0, &point) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_count(c, &sounds) == FVM_OK && sounds == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_at(c, 0, &info) == FVM_OK && info.sound_id == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_at(c, 1, &info) == FVM_NOT_FOUND && info.sound_id == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_info(c, 7, &info) == FVM_OK && info.format == 3 && info.sample_rate == 44100 &&
          info.channels == 1 && info.bits_per_sample == 16 && info.sample_count == 2 && info.data_size == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_read_pcm(c, 7, 0, 2, pcm, 1, &written) == FVM_INVALID_ARGUMENT && !written && pcm[0] == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_read_pcm(c, 7, 0, 2, pcm, 2, &written) == FVM_OK && written == 2 && pcm[0] == 1 &&
          pcm[1] == -2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_read_pcm(c, 7, 2, 0, 0, 0, &written) == FVM_OK && !written))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_read_pcm(c, 7, 3, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT && !written))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_info(c, 0, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_info(c, 8, &info) == FVM_NOT_FOUND && info.sound_id == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}

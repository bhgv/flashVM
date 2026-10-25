/* Prefix geometry, codec isolation, signed boundaries and detached compressed snapshots. */
#define main stream_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_sound.cpp"

#undef main


/* Construct independent wire bytes; the MPEG payload is intentionally not a real audio frame. */
static Bytes
mp3_movie(unsigned version, unsigned tag_id, unsigned stereo, unsigned samples, unsigned seek, size_t prefix = 4)
{
    Bytes tags, h = header(0x2e | stereo, 1), block;
    word(h, 65534);
    tag(tags, tag_id, h);
    word(block, samples);
    word(block, seek);
    block.resize(prefix);
    if (prefix == 4)
    {
        block.push_back(0xff);
        block.push_back(0x00);
        block.push_back(0x7e);
    }
    tag(tags, 19, block);
    return movie(tags, version);
}

#ifndef FVM_MP3_FIXTURE_ONLY


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundMP3Block view;
    view.struct_size = sizeof(view);
    view.frame = 99;
    if (!(fvm_swf_stream_sound_mp3_block_at(0, 0, 0, &view) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned seeks[] = {0, 1, 32767, 32768, 65534, 65535};
    const unsigned counts[] = {0, 1, 576, 1152, 65535};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned tag_id = 18; tag_id <= 45; tag_id += 27)
        {
            for (unsigned stereo = 0; stereo < 2; ++stereo)
            {
                for (unsigned c = 0; c < sizeof(counts) / sizeof(counts[0]); ++c)
                {
                    for (unsigned s = 0; s < sizeof(seeks) / sizeof(seeks[0]); ++s)
                    {
                        load(context, mp3_movie(version, tag_id, stereo, counts[c], seeks[s]));
                        if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(view.stream_index == 0 && view.block_index == 0 && view.frame == 1))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(view.declared_samples == counts[c]))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        int expected = (int)(seeks[s] >= 32768 ? (long)seeks[s] - 65536 : (long)seeks[s]);
                        if (!(view.seek_samples == expected && view.data_size == 3))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(view.data[0] == 0xff && view.data[1] == 0 && view.data[2] == 0x7e))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_SWFStreamSoundSnapshot *snapshot = 0;
                        if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_SWFStreamSoundStorageInfo charge;
                        charge.struct_size = sizeof(charge);
                        if (!(fvm_swf_stream_sound_snapshot_storage_info(snapshot, &charge) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_swf_stream_sound_snapshot_close(snapshot);
                        if (!(fvm_swf_stream_sound_snapshot_open_bounded(
                                  context, 0, charge.peak_bytes - 1, &snapshot) == FVM_OUT_OF_MEMORY &&
                              !snapshot))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_swf_stream_sound_snapshot_open_bounded(context, 0, charge.peak_bytes, &snapshot) ==
                              FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        Bytes empty;
                        load(context, movie(empty));
                        if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, 0, &view) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(view.declared_samples == counts[c] && view.seek_samples == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(view.data_size == 3 && view.data[2] == 0x7e))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        size_t written = 99;
                        short destination = 42;
                        if (!(fvm_swf_stream_sound_snapshot_read_pcm(
                                  snapshot, 0, (size_t)-1, 0, &destination, 1, &written) == FVM_UNSUPPORTED &&
                              !written && destination == 42))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_swf_stream_sound_snapshot_close(snapshot);
                    }
                }
            }
        }
    }
    /* Prefix failures leave the whole output byte-for-byte unchanged. */
    std::memset(&view, 0x5a, sizeof(view));
    view.struct_size = sizeof(view);
    FVM_SWFStreamSoundMP3Block saved = view;
    for (size_t prefix = 0; prefix < 4; ++prefix)
    {
        load(context, mp3_movie(4, 18, 0, 576, 65535, prefix));
        if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(&view, &saved, sizeof(view))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamSoundSnapshot *snapshot = 0;
        if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, 0, &view) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(&view, &saved, sizeof(view))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(snapshot);
    }
    /* Segment-relative indices and movie frame gaps are not MPEG sample positions. */
    {
        Bytes tags, empty, h = header(0x2f), payload;
        tag(tags, 45, header(12));
        word(h, 65534);
        tag(tags, 18, h);
        word(payload, 576);
        word(payload, 3);
        payload.push_back(0x7e);
        tag(tags, 19, payload);
        tag(tags, 1, empty);
        tag(tags, 1, empty);
        payload.clear();
        word(payload, 0);
        word(payload, 0);
        tag(tags, 19, payload);
        load(context, stream_movie(tags, 8, 3));
        if (!(fvm_swf_stream_sound_mp3_block_at(context, 1, 1, &view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view.stream_index == 1 && view.block_index == 1 && view.frame == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!view.declared_samples && !view.seek_samples && !view.data_size))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamSoundSnapshot *snapshot = 0;
        if (!(fvm_swf_stream_sound_snapshot_open(context, 1, &snapshot) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, 1, &view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view.stream_index == 1 && view.block_index == 1 && view.frame == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(snapshot);
        /* A later malformed root header still rejects an earlier valid prefix atomically. */
        tag(tags, 45, header(0x2e));
        load(context, stream_movie(tags, 8, 3));
        saved = view;
        if (!(fvm_swf_stream_sound_mp3_block_at(context, 1, 0, &view) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(&view, &saved, sizeof(view))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Structural parsing does not invent MPEG-frame validation or reject empty MPEG spans. */
    Bytes tags, h = header(0x2e), empty_payload;
    word(h, 0);
    tag(tags, 18, h);
    word(empty_payload, 0);
    word(empty_payload, 0);
    tag(tags, 19, empty_payload);
    load(context, movie(tags, 4));
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!view.declared_samples && !view.seek_samples && !view.data_size && view.data))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = view;
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 1, 0, &view) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&view, &saved, sizeof(view))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 1, &view) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view.struct_size = 0;
    if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view.struct_size = sizeof(view);
    /* Other encoded codecs retain their opaque bytes, never becoming accidental PCM. */
    FVM_SWFStreamSoundSnapshot *survivor = 0;
    for (unsigned format = 0; format < 16; ++format)
    {
        if (format == 2)
        {
            continue;
        }
        tags.clear();
        Bytes payload;
        payload.push_back(0);
        payload.push_back(255);
        tag(tags, 45, header((format << 4) | 14));
        tag(tags, 19, payload);
        load(context, movie(tags, 8));
        if (!(fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &view) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFStreamSoundSnapshot *snapshot = 0;
        if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, 0, &view) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_swf_stream_sound_snapshot_close(snapshot);
    }
    load(context, mp3_movie(8, 45, 1, 1152, 32768));
    if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &survivor) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(survivor, 0, &view) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(view.declared_samples == 1152 && view.seek_samples == -32768 && view.data_size == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = view;
    if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(survivor, 1, &view) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(&view, &saved, sizeof(view))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(survivor, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_mp3_block_at(0, 0, &view) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(survivor);
    return 0;
}

#endif

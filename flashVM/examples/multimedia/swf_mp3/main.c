/* C89 tutorial: export an MP3 StartSound or root stream after destroying its source VM. */
#include "flashvm/swf_sound_mixer.h" /* Adopt prepared voices after their VM has disappeared. */
#include "flashvm/swf_stream_pcm.h"  /* Native stream snapshots and incremental PCM reads. */
#include "example_constants.h"       /* Explicit source, ownership and finite-export limits. */
#include <stdio.h>                   /* Caller-selected binary input/output only. */
#include <stdlib.h>                  /* Own and release the bounded input file buffer. */
#include <string.h>                  /* Validate the explicit --stream switch before opening files. */


/* Report exact API errors and leave cleanup to the one owning main function. */
static int checked(FVM_Result result, const char *operation) /* No assertions in the tutorial. */
{                                                            /* Begin status translation. */
    if (result == FVM_OK)                                    /* Condition for this action: Continue only after acknowledged success. */
    {                                                        /* Begin conditional branch: Condition for this action: Continue only after acknowledged success. */
        return 1;                                            /* Continue only after acknowledged success. */
    } /* End conditional branch: Condition for this action: Continue only after acknowledged success. */
    fprintf(stderr, "%s: %d\n", operation, (int)result); /* Preserve the failing API status. */
    return 0;                                            /* Retain partial owners for centralized cleanup. */
} /* End status translation. */


/* Prepare source ownership on one thread; the export loop has no VM or device dependency. */
int main(int argc, char **argv)                                 /* INPUT.swf OUTPUT.pcm [--stream]. */
{                                                               /* Begin explicit file-export lifetime. */
    FVM_Config config;                                          /* Copied runtime configuration, bounded below. */
    FVM_Runtime *runtime = 0;                                   /* Source runtime, destroyed before export. */
    FVM_Context *context = 0;                                   /* Source context, destroyed before its runtime. */
    FVM_SWFSoundVoice *voice = 0;                               /* Native first StartSound interval/envelope/loop owner. */
    FVM_SWFStreamSoundSnapshot *source = 0;                     /* Native first MP3 stream PCM owner. */
    FVM_SWFSoundMixer *mixer = 0;                               /* Native owner of an adopted voice; no context is borrowed. */
    FVM_SWFSoundMixerConfig geometry;                           /* Explicit same-rate mixer output policy. */
    FVM_SWFStreamPCMReader reader;                              /* Caller cursor borrowing the normalized source. */
    FVM_SWFStreamPCMChunk chunk;                                /* Exact block/frame metadata for native stream copies. */
    FVM_SWFSoundVoiceInfo voice_info;                           /* Copied native event geometry. */
    FVM_SWFStreamSoundInfo stream_info;                         /* Copied normalized stream geometry. */
    FVM_SWFSoundEvent event;                                    /* Borrowed event metadata used only before teardown. */
    FVM_SWFSoundInfo sound;                                     /* Borrowed source codec metadata used only before teardown. */
    FILE *input = 0, *output = 0;                               /* Independent explicit native file owners. */
    unsigned char *bytes = 0;                                   /* Bounded input copy, released immediately after load. */
    long length = 0;                                            /* Validated ftell result before conversion to size_t. */
    short pcm[EXAMPLE_MP3_BLOCK * EXAMPLE_MP3_CHANNELS];        /* Caller-owned aligned transfer storage. */
    size_t written = 0, frames = 0;                             /* Channel-value count and derived complete frame count. */
    unsigned long expected_frames = 0;                          /* Explicit finite voice duration, excluding mixer silence. */
    unsigned long total = 0;                                    /* Finite accepted export count, checked before every write. */
    unsigned int rate = 0, channels = 0;                        /* Effective MPEG clock and native interleaving. */
    int stream = 0, status = 1;                                 /* Explicit source mode and pessimistic cleanup result. */
    FVM_Result result = FVM_OK;                                 /* Preserve EOF versus an actual decoder error. */
    if (argc == 1)                                              /* Usage performs no native or VM acquisition. */
    {                                                           /* Explain both executable modes. */
        puts("Usage: swf_mp3 INPUT.swf OUTPUT.pcm [--stream]"); /* No implicit output destination. */
        return 0;                                               /* A usage request succeeds. */
    } /* End usage-only mode. */
    if (argc == 4 && strcmp(argv[3], "--stream") == 0) /* Condition for this action: Choose first root stream. */
    {                                                  /* Begin conditional branch: Condition for this action: Choose first root stream. */
        stream = 1;                                    /* Choose first root stream. */
    } /* End conditional branch: Condition for this action: Choose first root stream. */
    else if (argc != 3) /* Reject malformed CLI before file side effects. */
    {                   /* Begin conditional branch: Reject malformed CLI before file side effects. */
        return 1;       /* Reject malformed CLI before file side effects. */
    } /* End conditional branch: Reject malformed CLI before file side effects. */
    input = fopen(argv[1], "rb");                 /* Open only the requested source. */
    if (!input || fseek(input, 0, SEEK_END) != 0) /* Condition for this action: Need a bounded regular file. */
    {                                             /* Begin conditional branch: Condition for this action: Need a bounded regular file. */
        goto cleanup;                             /* Need a bounded regular file. */
    } /* End conditional branch: Condition for this action: Need a bounded regular file. */
    length = ftell(input);                                              /* Inspect length before allocating. */
    if (length <= 0 || (unsigned long)length > EXAMPLE_MP3_INPUT_BYTES) /* Condition for this action: Bound input. */
    {                                                                   /* Begin conditional branch: Condition for this action: Bound input. */
        goto cleanup;                                                   /* Bound input. */
    } /* End conditional branch: Condition for this action: Bound input. */
    if (fseek(input, 0, SEEK_SET) != 0) /* Condition for this action: Rewind before the exact read. */
    {                                   /* Begin conditional branch: Condition for this action: Rewind before the exact read. */
        goto cleanup;                   /* Rewind before the exact read. */
    } /* End conditional branch: Condition for this action: Rewind before the exact read. */
    bytes = (unsigned char *)malloc((size_t)length);                        /* Own a disjoint temporary source buffer. */
    if (!bytes || fread(bytes, 1, (size_t)length, input) != (size_t)length) /* Condition for this action: No partial SWF. */
    {                                                                       /* Begin conditional branch: Condition for this action: No partial SWF. */
        goto cleanup;                                                       /* No partial SWF. */
    } /* End conditional branch: Condition for this action: No partial SWF. */
    if (fclose(input) != 0) /* Condition for this action: Treat close errors as terminal. */
    {                       /* Begin conditional branch: Condition for this action: Treat close errors as terminal. */
        input = 0;          /* Treat close errors as terminal. */
        goto cleanup;       /* Treat close errors as terminal. */
    } /* Treat close errors as terminal. */
    input = 0;                                        /* Acknowledge file release once. */
    if (!checked(fvm_config_init(&config), "config")) /* Condition for this action: Initialize owner policy. */
    {                                                 /* Begin conditional branch: Condition for this action: Initialize owner policy. */
        goto cleanup;                                 /* Initialize owner policy. */
    } /* End conditional branch: Condition for this action: Initialize owner policy. */
    config.memory_limit_bytes = EXAMPLE_MP3_NATIVE_BYTES;           /* Bound runtime payload preparation. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Condition for this action: Acquire VM. */
    {                                                               /* Begin conditional branch: Condition for this action: Acquire VM. */
        goto cleanup;                                               /* Acquire VM. */
    } /* End conditional branch: Condition for this action: Acquire VM. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Condition for this action: Acquire source. */
    {                                                               /* Begin conditional branch: Condition for this action: Acquire source. */
        goto cleanup;                                               /* Acquire source. */
    } /* End conditional branch: Condition for this action: Acquire source. */
    if (!checked(fvm_load_memory(context, bytes, (size_t)length), "load")) /* Condition for this action: Copy/validate. */
    {                                                                      /* Begin conditional branch: Condition for this action: Copy/validate. */
        goto cleanup;                                                      /* Copy/validate. */
    } /* End conditional branch: Condition for this action: Copy/validate. */
    free(bytes);                                                                   /* The context does not borrow the caller's file buffer. */
    bytes = 0;                                                                     /* The context does not borrow the caller's file buffer. */
    if (stream)                                                                    /* Prepare the complete supported first root MP3 segment. */
    {                                                                              /* Begin native stream ownership. */
        stream_info.struct_size = sizeof(stream_info);                             /* Request exact metadata ABI. */
        if (!checked(fvm_swf_stream_sound_at(context, 0, &stream_info), "stream")) /* Condition for this action: Header. */
        {                                                                          /* Begin conditional branch: Condition for this action: Header. */
            goto cleanup;                                                          /* Header. */
        } /* End conditional branch: Condition for this action: Header. */
        if (stream_info.format != FVM_SWF_SOUND_MP3) /* Condition for this action: Keep this example MP3-specific. */
        {                                            /* Begin conditional branch: Condition for this action: Keep this example MP3-specific. */
            goto cleanup;                            /* Keep this example MP3-specific. */
        } /* End conditional branch: Condition for this action: Keep this example MP3-specific. */
        result = fvm_swf_stream_sound_snapshot_open_pcm_bounded(                                                /* Validate and decode before publication. */
                                                                context, 0, EXAMPLE_MP3_NATIVE_BYTES, &source); /* Bound peak object/index/encoded/PCM. */
        if (!checked(result, "stream preparation"))                                                             /* Condition for this action: Failure leaves no native owner. */
        {                                                                                                       /* Begin conditional branch: Condition for this action: Failure leaves no native owner. */
            goto cleanup;                                                                                       /* Failure leaves no native owner. */
        } /* End conditional branch: Condition for this action: Failure leaves no native owner. */
        if (!checked(fvm_swf_stream_sound_snapshot_info(source, &stream_info), "native geometry")) /* Condition for this action: Copy. */
        {                                                                                          /* Begin conditional branch: Condition for this action: Copy. */
            goto cleanup;                                                                          /* Copy. */
        } /* End conditional branch: Condition for this action: Copy. */
        rate = stream_info.sample_rate;                                          /* Effective MPEG geometry. */
        channels = stream_info.channels;                                         /* Effective MPEG geometry. */
        if (!checked(fvm_swf_stream_pcm_reader_init(&reader, source), "reader")) /* Condition for this action: Borrow. */
        {                                                                        /* Begin conditional branch: Condition for this action: Borrow. */
            goto cleanup;                                                        /* Borrow. */
        } /* End conditional branch: Condition for this action: Borrow. */
    } /* End native stream preparation. */
    else                                                                   /* Prepare the first root StartSound with its authored interval/style policy. */
    {                                                                      /* Begin native voice ownership. */
        event.struct_size = sizeof(event);                                 /* Exact metadata layouts. */
        sound.struct_size = sizeof(sound);                                 /* Exact metadata layouts. */
        if (!checked(fvm_swf_sound_event_at(context, 0, &event), "event")) /* Condition for this action: Root ordinal. */
        {                                                                  /* Begin conditional branch: Condition for this action: Root ordinal. */
            goto cleanup;                                                  /* Root ordinal. */
        } /* End conditional branch: Condition for this action: Root ordinal. */
        if (!checked(fvm_swf_sound_info(context, event.sound_id, &sound), "sound")) /* Condition for this action: Codec. */
        {                                                                           /* Begin conditional branch: Condition for this action: Codec. */
            goto cleanup;                                                           /* Codec. */
        } /* End conditional branch: Condition for this action: Codec. */
        if (sound.format != FVM_SWF_SOUND_MP3) /* Condition for this action: Do not silently choose another codec. */
        {                                      /* Begin conditional branch: Condition for this action: Do not silently choose another codec. */
            goto cleanup;                      /* Do not silently choose another codec. */
        } /* End conditional branch: Condition for this action: Do not silently choose another codec. */
        if (!checked(fvm_swf_sound_voice_open(context, 0, &voice), "voice")) /* Condition for this action: Own native PCM. */
        {                                                                    /* Begin conditional branch: Condition for this action: Own native PCM. */
            goto cleanup;                                                    /* Own native PCM. */
        } /* End conditional branch: Condition for this action: Own native PCM. */
        voice_info.struct_size = sizeof(voice_info);                                  /* Request copied progress/geometry. */
        if (!checked(fvm_swf_sound_voice_info(voice, &voice_info), "voice geometry")) /* Condition for this action: Copy. */
        {                                                                             /* Begin conditional branch: Condition for this action: Copy. */
            goto cleanup;                                                             /* Copy. */
        } /* End conditional branch: Condition for this action: Copy. */
        rate = voice_info.sample_rate;                                                      /* Effective source clock. */
        channels = voice_info.channels;                                                     /* Effective source clock. */
        if (voice_info.play_count && voice_info.frames_per_play >                           /* Check multiplication before adoption. */
                                         EXAMPLE_MP3_EXPORT_FRAMES / voice_info.play_count) /* Bound long repeats. */
        {                                                                                   /* Begin scope: Bound long repeats. */
            goto cleanup;                                                                   /* Bound long repeats. */
        } /* End scope: Bound long repeats. */
        expected_frames = (unsigned long)voice_info.frames_per_play * voice_info.play_count; /* Finite source. */
    } /* End native voice preparation. */
    fvm_context_destroy(context);                                                        /* No managed source is needed by either native owner. */
    context = 0;                                                                         /* No managed source is needed by either native owner. */
    fvm_runtime_destroy(runtime);                                                        /* Complete VM teardown before file output starts. */
    runtime = 0;                                                                         /* Complete VM teardown before file output starts. */
    if (!stream)                                                                         /* Native command publication no longer needs a live context. */
    {                                                                                    /* Begin caller-controlled mixer ownership. */
        geometry.struct_size = sizeof(geometry);                                         /* Exact configuration ABI. */
        geometry.sample_rate = rate;                                                     /* Keep source clock/interleaving. */
        geometry.channels = channels;                                                    /* Keep source clock/interleaving. */
        geometry.voice_capacity = EXAMPLE_MP3_VOICES;                                    /* Allow MPEG rates. */
        geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT;                                    /* Allow MPEG rates. */
        if (!checked(fvm_swf_sound_mixer_create_configured(&geometry, &mixer), "mixer")) /* Condition for this action: Own. */
        {                                                                                /* Begin conditional branch: Condition for this action: Own. */
            goto cleanup;                                                                /* Own. */
        } /* End conditional branch: Condition for this action: Own. */
        if (!checked(fvm_swf_sound_mixer_submit_prepared(mixer, &voice), "adopt")) /* Condition for this action: Consume handle. */
        {                                                                          /* Begin conditional branch: Condition for this action: Consume handle. */
            goto cleanup;                                                          /* Consume handle. */
        } /* End conditional branch: Condition for this action: Consume handle. */
    } /* End native command publication; voice is null on success. */
    output = fopen(argv[2], "wb"); /* Explicitly replace only the requested PCM output. */
    if (!output)                   /* Condition for this action: A native file failure is terminal, never an atomic queue retry. */
    {                              /* Begin conditional branch: Condition for this action: A native file failure is terminal, never an atomic queue retry. */
        goto cleanup;              /* A native file failure is terminal, never an atomic queue retry. */
    } /* End conditional branch: Condition for this action: A native file failure is terminal, never an atomic queue retry. */
    for (;;)                                                                                                                          /* Copy only finite native chunks; never execute a timeline or infer a movie clock. */
    {                                                                                                                                 /* Begin the VM-free export loop. */
        chunk.struct_size = sizeof(chunk);                                                                                            /* Native stream metadata for this copy. */
        if (stream)                                                                                                                   /* Condition for this action: Skip no encoded block identities. */
        {                                                                                                                             /* Begin conditional branch: Condition for this action: Skip no encoded block identities. */
            result = fvm_swf_stream_pcm_reader_next(                                                                                  /* Skip no encoded block identities. */
                                                    &reader, EXAMPLE_MP3_BLOCK, pcm, sizeof(pcm) / sizeof(pcm[0]), &chunk, &written); /* Values. */
        } /* End conditional branch: Condition for this action: Skip no encoded block identities. */
        else                              /* Bound mixer output to the prepared voice duration; never export an infinite silence tail. */
        {                                 /* Read only caller-authorized native frames. */
            if (total == expected_frames) /* Condition for this action: The host owns finite output policy. */
            {                             /* Begin conditional branch: Condition for this action: The host owns finite output policy. */
                break;                    /* The host owns finite output policy. */
            } /* End conditional branch: Condition for this action: The host owns finite output policy. */
            frames = (size_t)(expected_frames - total); /* Remaining explicit source duration. */
            if (frames > EXAMPLE_MP3_BLOCK)             /* Condition for this action: Bound each native call. */
            {                                           /* Begin conditional branch: Condition for this action: Bound each native call. */
                frames = EXAMPLE_MP3_BLOCK;             /* Bound each native call. */
            } /* End conditional branch: Condition for this action: Bound each native call. */
            result = fvm_swf_sound_mixer_read(mixer, frames, pcm,                      /* Advance only adopted native voices. */
                                              sizeof(pcm) / sizeof(pcm[0]), &written); /* Capacity counts channel values. */
        } /* End the finite native mixer copy. */
        if (result == FVM_NOT_FOUND) /* Condition for this action: Only native EOF completes a successful export. */
        {                            /* Begin conditional branch: Condition for this action: Only native EOF completes a successful export. */
            break;                   /* Only native EOF completes a successful export. */
        } /* End conditional branch: Condition for this action: Only native EOF completes a successful export. */
        if (!checked(result, "native read")) /* Condition for this action: Keep source alive through cleanup. */
        {                                    /* Begin conditional branch: Condition for this action: Keep source alive through cleanup. */
            goto cleanup;                    /* Keep source alive through cleanup. */
        } /* End conditional branch: Condition for this action: Keep source alive through cleanup. */
        frames = written / channels;                    /* Geometry was validated during native preparation. */
        if (frames > EXAMPLE_MP3_EXPORT_FRAMES - total) /* Condition for this action: Bound authored long loops. */
        {                                               /* Begin conditional branch: Condition for this action: Bound authored long loops. */
            goto cleanup;                               /* Bound authored long loops. */
        } /* End conditional branch: Condition for this action: Bound authored long loops. */
        if (fwrite(pcm, sizeof(pcm[0]), written, output) != written) /* Condition for this action: Terminal short write. */
        {                                                            /* Begin conditional branch: Condition for this action: Terminal short write. */
            goto cleanup;                                            /* Terminal short write. */
        } /* End conditional branch: Condition for this action: Terminal short write. */
        total += (unsigned long)frames; /* Advance after complete output acceptance. */
    } /* End native-only export. */
    if (mixer && !checked(fvm_swf_sound_mixer_stop_id(mixer, voice_info.sound_id), "stop ID")) /* Condition for this action: No device. */
    {                                                                                          /* Begin conditional branch: Condition for this action: No device. */
        goto cleanup;                                                                          /* No device. */
    } /* End conditional branch: Condition for this action: No device. */
    if (fclose(output) != 0) /* Condition for this action: Flush failure remains observable. */
    {                        /* Begin conditional branch: Condition for this action: Flush failure remains observable. */
        output = 0;          /* Flush failure remains observable. */
        goto cleanup;        /* Flush failure remains observable. */
    } /* Flush failure remains observable. */
    output = 0;                                                                                         /* File ownership ended exactly once. */
    printf("Exported %lu MP3 frames at %u Hz/%u channels after VM teardown.\n", total, rate, channels); /* Evidence. */
    status = 0;                                                                                         /* Both native EOF and output close succeeded. */
cleanup:                                                                                                /* Readers stop before their borrowed native source is released. */
    if (input)                                                                                          /* Condition for this action: Close partial input acquisition. */
    {                                                                                                   /* Begin conditional branch: Condition for this action: Close partial input acquisition. */
        fclose(input);                                                                                  /* Close partial input acquisition. */
    } /* End conditional branch: Condition for this action: Close partial input acquisition. */
    if (output)         /* Condition for this action: Close partial output; a failed export may leave a partial file. */
    {                   /* Begin conditional branch: Condition for this action: Close partial output; a failed export may leave a partial file. */
        fclose(output); /* Close partial output; a failed export may leave a partial file. */
    } /* End conditional branch: Condition for this action: Close partial output; a failed export may leave a partial file. */
    free(bytes);                                 /* Null-safe temporary file-buffer release. */
    fvm_swf_sound_mixer_close(mixer);            /* Release adopted ownership after every native read stops. */
    fvm_swf_sound_voice_close(voice);            /* Null-safe native event release, no VM required. */
    fvm_swf_stream_sound_snapshot_close(source); /* Release native stream after reader quiescence. */
    if (context)                                 /* Condition for this action: Release partial managed preparation. */
    {                                            /* Begin conditional branch: Condition for this action: Release partial managed preparation. */
        fvm_context_destroy(context);            /* Release partial managed preparation. */
    } /* End conditional branch: Condition for this action: Release partial managed preparation. */
    if (runtime)                      /* Condition for this action: End the remaining runtime last. */
    {                                 /* Begin conditional branch: Condition for this action: End the remaining runtime last. */
        fvm_runtime_destroy(runtime); /* End the remaining runtime last. */
    } /* End conditional branch: Condition for this action: End the remaining runtime last. */
    return status; /* Nonzero reports every terminal preparation or file error. */
} /* End bounded MP3 asset export. */

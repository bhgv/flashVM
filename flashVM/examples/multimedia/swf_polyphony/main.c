/* Tutorial: independent real file sinks demonstrate bound StartSound voice synchronization. */
#include "flashvm/swf_sound.h" /* Import event output and context-owned audio channels. */
#include "example_constants.h" /* Name fixture event ordinals and independent output counts. */
#include <stdio.h>             /* Use actual host file creation, writes, closing and diagnostics. */
#include <string.h>            /* Initialize host state and reject identical output path strings. */

/* Keep each output's native FILE alive independently until its own stop callback. */
typedef struct VoiceFile       /* Retain host resources outside the VM managed heap. */
{                              /* Begin one independently stoppable output. */
    const char *path;          /* Borrow an explicit command-line path until cleanup. */
    FILE *file;                /* Own the lazily opened file, including after a failed native write. */
    unsigned int writes;       /* Count actual output callback attempts. */
    unsigned int stops;        /* Count actual stop callbacks for this output. */
    FVM_AudioChannel *channel; /* Borrow the context-owned registration until context destruction. */
} VoiceFile;                   /* Finish host-owned output state. */


/* Consume one complete borrowed S16 voice through the actual C runtime. */
static FVM_Result write_voice(void *raw, const FVM_MediaFrame *frame) /* Native output remains synchronous. */
{                                                                     /* Begin real PCM file output. */
    VoiceFile *voice = (VoiceFile *)raw;                              /* Recover this independent host sink. */
    ++voice->writes;                                                  /* Make no-multiple suppression observable. */
    if (!voice->file)                                                 /* Open no file before a real accepted voice submission. */
    {                                                                 /* Begin conditional branch: Open no file before a real accepted voice submission. */
        voice->file = fopen(voice->path, "wb");                       /* Create or replace only the explicitly selected output. */
    } /* End conditional branch: Open no file before a real accepted voice submission. */
    if (!voice->file || fwrite(frame->data, 1, frame->size, voice->file) != frame->size) /* Require complete native output. */
    {                                                                                    /* Begin conditional branch: Require complete native output. */
        return FVM_ERROR;                                                                /* Retain any opened FILE for ordinary host cleanup. */
    } /* End conditional branch: Require complete native output. */
    return FVM_OK; /* Acknowledge the complete copied voice buffer. */
} /* Finish real PCM output. */


/* Stop one voice without affecting the other independent FILE objects. */
static FVM_Result stop_voice(void *raw)  /* The VM preserves failed stops for retry. */
{                                        /* Begin actual native resource closing. */
    VoiceFile *voice = (VoiceFile *)raw; /* Recover this output's ownership. */
    int result = 0;                      /* Treat an absent FILE as already closed. */
    ++voice->stops;                      /* Make resource-selective stopping observable. */
    if (voice->file)                     /* Close only acquired native resources. */
    {                                    /* Invalidate the stored handle even when fclose reports an I/O error. */
        result = fclose(voice->file);    /* Flush and close the real host stream. */
        voice->file = 0;                 /* A subsequent retry must not close an invalid FILE. */
    } /* Finish native close ownership. */
    return result == 0 ? FVM_OK : FVM_ERROR; /* Preserve actual host I/O failure. */
} /* Finish independent voice stopping. */


/* Report API errors with ordinary control flow rather than tutorial assertions. */
static int checked(FVM_Result result, const char *operation) /* Keep partial initialization recoverable. */
{                                                            /* Begin status reporting. */
    if (result == FVM_OK)                                    /* Continue only after acknowledged success. */
    {                                                        /* Begin conditional branch: Continue only after acknowledged success. */
        return 1;                                            /* Let the caller proceed. */
    } /* End conditional branch: Continue only after acknowledged success. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Name the failing operation and status. */
    return 0;                                                   /* Ask the caller to use centralized cleanup. */
} /* Finish status reporting. */

/* Supply two tiny U8 resources and four independent root StartSound controls. */
static const unsigned char swf[] = /* Preserve literal wire bytes as an executable tutorial fixture. */
    { /* Begin scope: Preserve literal wire bytes as an executable tutorial fixture. */
        /* Begin an uncompressed SWF8 with one declared frame and exact length 60. */
        'F', 'W', 'S', 8, 60, 0, 0, 0, 8, 0, 0, 12, 1, 0, /* Encode signature, size, RECT, rate and frame count. */
        0x89, 3, 7, 0, 12, 2, 0, 0, 0, 0, 255,            /* DefineSound 7: mono U8, 44100 Hz, two sample frames. */
        0x89, 3, 8, 0, 12, 2, 0, 0, 0, 0, 255,            /* DefineSound 8: same samples, separate resource identity. */
        0xc3, 3, 7, 0, 0,                                 /* Event 0 starts another instance of resource 7. */
        0xc3, 3, 7, 0, 16,                                /* Event 1 requests no-multiple for resource 7. */
        0xc3, 3, 7, 0, 32,                                /* Event 2 stops every bound resource-7 voice in this context. */
        0xc3, 3, 8, 0, 0,                                 /* Event 3 starts the independent resource 8. */
        0x40, 0, 0, 0                                     /* End the frame and root tag stream. */
}; /* Finish the canonical input bytes. */


/* Drive overlap and synchronization explicitly, without a hidden playback clock or mixer. */
int main(int argc, char **argv)                                                                                                    /* Keep the entire example compatible with C89. */
{                                                                                                                                  /* Begin explicit host and VM ownership. */
    FVM_Config config;                                                                                                             /* Initialize the embedding configuration before runtime creation. */
    FVM_Runtime *runtime = 0;                                                                                                      /* Own the runtime after successful creation. */
    FVM_Context *context = 0;                                                                                                      /* Own canonical input and registered channels. */
    VoiceFile voices[TUTORIAL_SWF_VOICE_COUNT];                                                                                    /* Keep all host userdata alive until context cleanup returns. */
    FVM_MediaOutput output;                                                                                                        /* Copy callback tables into each registration. */
    size_t i;                                                                                                                      /* Traverse the three independent output registrations. */
    int active, exit_code = 1;                                                                                                     /* Track observable state and ordinary process status. */
    memset(voices, 0, sizeof(voices));                                                                                             /* Make cleanup safe after any partial setup. */
    if (argc != TUTORIAL_SWF_ARGUMENT_COUNT)                                                                                       /* Require explicit paths before any native file operation. */
    {                                                                                                                              /* Explain the real output side effects. */
        printf("Usage: %s VOICE_A.pcm VOICE_B.pcm OTHER.pcm\nEach named output may be replaced; raw native S16 only.\n", argv[0]); /* Keep smoke runs free of files and devices. */
        return 0;                                                                                                                  /* Do not create a VM or output without explicit paths. */
    } /* Finish the explicit output gate. */
    if (!strcmp(argv[1], argv[2]) || !strcmp(argv[1], argv[3]) || !strcmp(argv[2], argv[3])) /* Reject identical path strings before opening anything. */
    {                                                                                        /* Require independent host sinks for independent voices. */
        fprintf(stderr, "Choose three distinct output files.\n");                            /* Host paths must also avoid aliases to the same file. */
        return 1;                                                                            /* No native output or VM has been acquired. */
    } /* Finish output path validation. */
    if (!checked(fvm_config_init(&config), "config_init") || !checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Initialize runtime ownership in dependency order. */
    {                                                                                                                           /* Begin conditional branch: Initialize runtime ownership in dependency order. */
        goto cleanup;                                                                                                           /* Release any acquired host/runtime resources normally. */
    } /* End conditional branch: Initialize runtime ownership in dependency order. */
    if (!checked(fvm_context_create(runtime, &context), "context_create") || !checked(fvm_load_memory(context, swf, sizeof(swf)), "load_memory")) /* Own canonical SWF bytes before selecting controls. */
    {                                                                                                                                             /* Begin conditional branch: Own canonical SWF bytes before selecting controls. */
        goto cleanup;                                                                                                                             /* No output file exists yet. */
    } /* End conditional branch: Own canonical SWF bytes before selecting controls. */
    for (i = 0; i < TUTORIAL_SWF_VOICE_COUNT; ++i)                                                      /* Register separate voice outputs without activating them. */
    {                                                                                                   /* Initialize one independently owned sink. */
        voices[i].path = argv[i + 1];                                                                   /* Borrow its caller-selected output path. */
        output.struct_size = sizeof(output);                                                            /* Supply the exact callback-table ABI. */
        output.userdata = &voices[i];                                                                   /* Keep native FILE ownership outside managed GC. */
        output.write = write_voice;                                                                     /* Delegate actual output to fwrite. */
        output.stop = stop_voice;                                                                       /* Delegate discard/resource closing to fclose. */
        if (!checked(fvm_audio_channel_create(context, &output, &voices[i].channel), "channel_create")) /* Copy the table into an inactive registration. */
        {                                                                                               /* Begin conditional branch: Copy the table into an inactive registration. */
            goto cleanup;                                                                               /* Context cleanup releases earlier registrations. */
        } /* End conditional branch: Copy the table into an inactive registration. */
    } /* Finish output registration. */
    if (!checked(fvm_swf_sound_event_submit(context, TUTORIAL_SWF_EVENT_START, voices[0].channel), "first_voice") || !checked(fvm_swf_sound_event_submit(context, TUTORIAL_SWF_EVENT_START, voices[1].channel), "overlap_voice")) /* Start two resource-7 voices on separate channels. */
    {                                                                                                                                                                                                                             /* Begin conditional branch: Start two resource-7 voices on separate channels. */
        goto cleanup;                                                                                                                                                                                                             /* Preserve host state through terminal cleanup on failure. */
    } /* End conditional branch: Start two resource-7 voices on separate channels. */
    if (!checked(fvm_swf_sound_event_submit(context, TUTORIAL_SWF_EVENT_NO_MULTIPLE, voices[2].channel), "no_multiple")) /* Detect resource 7 across all bound active voices. */
    {                                                                                                                    /* Begin conditional branch: Detect resource 7 across all bound active voices. */
        goto cleanup;                                                                                                    /* Suppression is successful and does not bind this unused channel. */
    } /* End conditional branch: Detect resource 7 across all bound active voices. */
    printf("After no-multiple: A=%u writes, B=%u writes, unused=%u writes\n", voices[0].writes, voices[1].writes, voices[2].writes);                                                                                                  /* Expected counts are 1, 1 and 0. */
    if (!checked(fvm_swf_sound_event_submit(context, TUTORIAL_SWF_EVENT_OTHER, voices[2].channel), "other_resource") || !checked(fvm_swf_sound_event_submit(context, TUTORIAL_SWF_EVENT_STOP, voices[0].channel), "stop_resource_7")) /* Reuse the unbound sink for resource 8, then stop both resource-7 voices. */
    {                                                                                                                                                                                                                                 /* Begin conditional branch: Reuse the unbound sink for resource 8, then stop both resource-7 voices. */
        goto cleanup;                                                                                                                                                                                                                 /* Resource 8 must remain independently active. */
    } /* End conditional branch: Reuse the unbound sink for resource 8, then stop both resource-7 voices. */
    if (!checked(fvm_audio_channel_active(voices[2].channel, &active), "other_active")) /* Observe state without entering host callbacks. */
    {                                                                                   /* Begin conditional branch: Observe state without entering host callbacks. */
        goto cleanup;                                                                   /* Keep failed query handling explicit. */
    } /* End conditional branch: Observe state without entering host callbacks. */
    printf("After resource stop: A=%u stops, B=%u stops, other active=%d\n", voices[0].stops, voices[1].stops, active); /* Expected values are 1, 1 and 1. */
    if (!checked(fvm_audio_stop_all(context), "stop_all"))                                                              /* Close the remaining resource-8 output through its real callback. */
    {                                                                                                                   /* Begin conditional branch: Close the remaining resource-8 output through its real callback. */
        goto cleanup;                                                                                                   /* Terminal cleanup can retry failed native stops. */
    } /* End conditional branch: Close the remaining resource-8 output through its real callback. */
    exit_code = 0;                    /* All requested native writes and stops succeeded. */
cleanup:                              /* Preserve every userdata object until the context finishes final callbacks. */
    if (context)                      /* Destroy only a successfully acquired context. */
    {                                 /* Begin conditional branch: Destroy only a successfully acquired context. */
        fvm_context_destroy(context); /* Stop active registrations and invalidate their borrowed handles. */
    } /* End conditional branch: Destroy only a successfully acquired context. */
    for (i = 0; i < TUTORIAL_SWF_VOICE_COUNT; ++i) /* Recover any FILE left outside acknowledged active state after failed writes. */
    {                                              /* Begin loop body: Recover any FILE left outside acknowledged active state after failed writes. */
        if (voices[i].file)                        /* Do not close absent or already stopped resources. */
        {                                          /* Begin conditional branch: Do not close absent or already stopped resources. */
            fclose(voices[i].file);                /* Release native output after all context callbacks are complete. */
        } /* End conditional branch: Do not close absent or already stopped resources. */
    } /* End loop body: Recover any FILE left outside acknowledged active state after failed writes. */
    if (runtime)                      /* Runtime must outlive its context. */
    {                                 /* Begin conditional branch: Runtime must outlive its context. */
        fvm_runtime_destroy(runtime); /* Release VM ownership last. */
    } /* End conditional branch: Runtime must outlive its context. */
    return exit_code; /* Report ordinary status without assertions or hidden device access. */
} /* Finish explicit polyphonic control demonstration. */

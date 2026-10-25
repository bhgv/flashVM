/* Tutorial: register an observable PCM file sink and stop it through real AVM1 StopSounds. */
#include "flashvm/multimedia.h" /* Declare context-owned channels and borrowed PCM frames. */
#include "flashvm/avm1.h"       /* Execute the same StopSounds opcode found in SWF actions. */
#include <stdio.h>              /* Implement host output using actual fwrite and fclose calls. */
#include <string.h>             /* Initialize the frame before publishing explicit audio fields. */

typedef struct FileOutput       /* Own the host resource independently of the VM channel wrapper. */
{                               /* Begin host-owned output state. */
    const char *path;           /* Borrow argv storage for the complete program lifetime. */
    FILE *file;                 /* Own the explicitly opened PCM file until stop or cleanup. */
    unsigned int stops;         /* Count actual file-closing stop callbacks. */
} FileOutput;                   /* Finish the host state. */


/* Submit native-endian S16 samples to a real observable file, not a sound card. */
static FVM_Result write_pcm(void *raw, const FVM_MediaFrame *frame) /* Borrow the validated frame only during this callback. */
{                                                                   /* Begin host output submission. */
    FileOutput *host = (FileOutput *)raw;                           /* Recover the still-live host state. */
    if (!host->file)                                                /* Reopen after a previous successful stop if another write is requested. */
    {                                                               /* Begin conditional branch: Reopen after a previous successful stop if another write is requested. */
        host->file = fopen(host->path, "ab");                       /* Append raw PCM to the explicitly requested output path. */
    } /* End conditional branch: Reopen after a previous successful stop if another write is requested. */
    if (!host->file)      /* Report native open failure without publishing active playback. */
    {                     /* Begin conditional branch: Report native open failure without publishing active playback. */
        return FVM_ERROR; /* The host owns cleanup of any partial resources. */
    } /* End conditional branch: Report native open failure without publishing active playback. */
    if (fwrite(frame->data, 1, frame->size, host->file) != frame->size) /* Consume every byte synchronously. */
    {                                                                   /* Begin conditional branch: Consume every byte synchronously. */
        return FVM_ERROR;                                               /* Keep the file owned by host cleanup after a partial I/O failure. */
    } /* End conditional branch: Consume every byte synchronously. */
    return FVM_OK; /* Acknowledged submission makes the registration active. */
} /* Finish file submission. */


/* Stop this sink by closing its actual native resource; a real audio bridge would discard its device queue. */
static FVM_Result stop_pcm(void *raw)     /* Borrow the host state through the close call. */
{                                         /* Begin the host stopping operation. */
    FileOutput *host = (FileOutput *)raw; /* Recover the same state used by write. */
    int result = 0;                       /* Preserve the actual fclose result. */
    if (host->file)                       /* Avoid closing an already stopped sink. */
    {                                     /* Close the one currently owned native resource. */
        result = fclose(host->file);      /* Flush and close the file using the real system C library. */
        host->file = 0;                   /* fclose consumes the FILE even when it reports an error. */
    } /* Finish native resource closure. */
    ++host->stops;                           /* Make StopSounds delivery visible without assertions. */
    return result == 0 ? FVM_OK : FVM_ERROR; /* Propagate the host I/O status. */
} /* Finish host stopping. */


/* Report ordinary API errors while keeping cleanup centralized. */
static int checked(FVM_Result result, const char *operation) /* Borrow the operation label for diagnostics. */
{                                                            /* Begin ordinary error reporting. */
    if (result == FVM_OK)                                    /* Accept only acknowledged success. */
    {                                                        /* Begin conditional branch: Accept only acknowledged success. */
        return 1;                                            /* Allow the tutorial to continue. */
    } /* End conditional branch: Accept only acknowledged success. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Explain the failed operation. */
    return 0;                                                   /* Route control through cleanup. */
} /* Finish error reporting. */


/* Run without opening any device or file unless the caller provides an output pathname. */
int main(int argc, char **argv)                                                                                     /* Keep the executable compatible with C89 hosts. */
{                                                                                                                   /* Begin explicit ownership of host and VM resources. */
    FVM_Config config;                                                                                              /* Initialize runtime configuration before creating handles. */
    FVM_Runtime *runtime = 0;                                                                                       /* Track partial runtime ownership. */
    FVM_Context *context = 0;                                                                                       /* Own the native channel registration. */
    FVM_AudioChannel *channel = 0;                                                                                  /* Borrow this handle until successful close or context destruction. */
    FVM_MediaOutput output;                                                                                         /* Supply callbacks which creation copies immediately. */
    FVM_MediaFrame frame;                                                                                           /* Borrow the local PCM bytes for synchronous submission. */
    const unsigned char stop[] = {0x09, 0x00};                                                                      /* Encode StopSounds followed by End, without loading a SWF. */
    short samples[2] = {0, 0};                                                                                      /* Provide two silent mono S16 sample frames. */
    FileOutput host;                                                                                                /* Keep callback userdata alive until context destruction has returned. */
    int active = 0;                                                                                                 /* Query the last acknowledged output state. */
    int exit_code = 1;                                                                                              /* Report success only after all demonstrated operations complete. */
    memset(&host, 0, sizeof(host));                                                                                 /* Make partial-setup cleanup safe. */
    if (argc != 2)                                                                                                  /* Require an explicit path before creating output. */
    {                                                                                                               /* Explain usage without opening native resources. */
        printf("Usage: %s OUTPUT.pcm\nFile sink only; no audible output. Existing files are appended.\n", argv[0]); /* Describe the observable side effect. */
        return 0;                                                                                                   /* Treat the no-argument usage path as ordinary success. */
    } /* Finish the no-output usage path. */
    host.path = argv[1];                                   /* Borrow the explicitly supplied filename. */
    if (!checked(fvm_config_init(&config), "config_init")) /* Initialize the public configuration ABI. */
    {                                                      /* Begin conditional branch: Initialize the public configuration ABI. */
        goto cleanup;                                      /* No runtime exists yet. */
    } /* End conditional branch: Initialize the public configuration ABI. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Acquire the runtime. */
    {                                                                      /* Begin conditional branch: Acquire the runtime. */
        goto cleanup;                                                      /* Release only successfully created handles. */
    } /* End conditional branch: Acquire the runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context_create")) /* Acquire the channel owner context. */
    {                                                                      /* Begin conditional branch: Acquire the channel owner context. */
        goto cleanup;                                                      /* Keep runtime alive for context cleanup. */
    } /* End conditional branch: Acquire the channel owner context. */
    output.struct_size = sizeof(output);                                                  /* Declare the complete callback-table layout. */
    output.userdata = &host;                                                              /* Borrow native state; no managed objects are captured. */
    output.write = write_pcm;                                                             /* Delegate PCM submission to actual file output. */
    output.stop = stop_pcm;                                                               /* Delegate StopSounds to actual file closure. */
    if (!checked(fvm_audio_channel_create(context, &output, &channel), "channel_create")) /* Register an inactive output. */
    {                                                                                     /* Begin conditional branch: Register an inactive output. */
        goto cleanup;                                                                     /* Registration never opens the file itself. */
    } /* End conditional branch: Register an inactive output. */
    memset(&frame, 0, sizeof(frame));                                        /* Clear fields unused by audio. */
    frame.kind = FVM_MEDIA_AUDIO;                                            /* Select native-endian signed-16 PCM. */
    frame.data = (const unsigned char *)samples;                             /* Borrow the local sample array synchronously. */
    frame.size = sizeof(samples);                                            /* Publish the complete accessible byte range. */
    frame.channels = 1;                                                      /* Use one interleaved channel. */
    frame.sample_rate = 8000;                                                /* Declare samples per second. */
    frame.samples = 2;                                                       /* Count sample frames rather than bytes. */
    if (!checked(fvm_audio_channel_write(channel, &frame), "channel_write")) /* Open and write the actual PCM file. */
    {                                                                        /* Begin conditional branch: Open and write the actual PCM file. */
        goto cleanup;                                                        /* Preserve host resource ownership even after failed output. */
    } /* End conditional branch: Open and write the actual PCM file. */
    if (!checked(fvm_avm1_execute(context, stop, sizeof(stop)), "StopSounds")) /* Stop every active registered output in this context. */
    {                                                                          /* Begin conditional branch: Stop every active registered output in this context. */
        goto cleanup;                                                          /* Other contexts and standalone output bridges remain independent. */
    } /* End conditional branch: Stop every active registered output in this context. */
    if (!checked(fvm_audio_channel_active(channel, &active), "channel_active")) /* Inspect copied state without entering host code. */
    {                                                                           /* Begin conditional branch: Inspect copied state without entering host code. */
        goto cleanup;                                                           /* Do not read an invalid query result. */
    } /* End conditional branch: Inspect copied state without entering host code. */
    printf("Active: %d; stop callbacks: %u; appended bytes: %lu\n", active, host.stops, (unsigned long)sizeof(samples)); /* Report observable results. */
    if (!checked(fvm_audio_channel_close(channel), "channel_close"))                                                     /* Unregister after the acknowledged stop. */
    {                                                                                                                    /* Begin conditional branch: Unregister after the acknowledged stop. */
        goto cleanup;                                                                                                    /* A failed close would keep the handle valid for retry. */
    } /* End conditional branch: Unregister after the acknowledged stop. */
    channel = 0;                      /* Never use the successfully closed handle again. */
    exit_code = 0;                    /* Every demonstrated operation completed. */
cleanup:                              /* Keep native userdata alive through any terminal stop callback. */
    if (context)                      /* Destroy only a successfully created context. */
    {                                 /* Begin conditional branch: Destroy only a successfully created context. */
        fvm_context_destroy(context); /* Stop remaining active outputs and invalidate all channel handles. */
    } /* End conditional branch: Destroy only a successfully created context. */
    if (host.file)         /* A failed write can leave a host resource which the registration never acknowledged. */
    {                      /* Begin conditional branch: A failed write can leave a host resource which the registration never acknowledged. */
        fclose(host.file); /* Release any such resource explicitly after context callbacks are finished. */
    } /* End conditional branch: A failed write can leave a host resource which the registration never acknowledged. */
    if (runtime)                      /* Runtime must outlive its context and terminal output callbacks. */
    {                                 /* Begin conditional branch: Runtime must outlive its context and terminal output callbacks. */
        fvm_runtime_destroy(runtime); /* Release the remaining runtime storage. */
    } /* End conditional branch: Runtime must outlive its context and terminal output callbacks. */
    return exit_code; /* Return an ordinary status without assertions or abort. */
} /* Finish the channel tutorial. */

/* Tutorial: explicitly open a Linux ALSA endpoint and stop it through the VM channel API. */
#include "flashvm/multimedia.h"          /* Declare native ALSA output and context registration. */
#include "flashvm/avm1.h"                /* Execute real StopSounds after explicit host output. */
#include <stdio.h>                       /* Print instructions and wait for explicit user input before stopping. */
#include <string.h>                      /* Clear unused frame fields before submission. */
#define TUTORIAL_ALSA_RATE 8000U         /* Submit mono PCM at eight thousand sample frames per second. */
#define TUTORIAL_ALSA_SAMPLES 800U       /* Generate one tenth of a second for the explicit device demonstration. */
#define TUTORIAL_ALSA_LATENCY_US 200000U /* Request enough buffering for the complete short frame. */
#define TUTORIAL_ALSA_HALF_PERIOD 16U    /* Alternate polarity every sixteen samples for a 250 Hz square tone. */
#define TUTORIAL_ALSA_AMPLITUDE 4000     /* Use a modest signed-16 amplitude for the explicitly requested output. */


/* Report normal API failures without aborting the host process. */
static int checked(FVM_Result result, const char *operation) /* Borrow the diagnostic label during this call. */
{                                                            /* Begin ordinary status handling. */
    if (result == FVM_OK)                                    /* Acknowledge only successful provider or VM operations. */
    {                                                        /* Begin conditional branch: Acknowledge only successful provider or VM operations. */
        return 1;                                            /* Continue the tutorial. */
    } /* End conditional branch: Acknowledge only successful provider or VM operations. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Keep unsupported builds and device errors visible. */
    return 0;                                                   /* Select centralized cleanup. */
} /* Finish status handling. */


/* Require an explicit ALSA device name before opening or producing sound. */
int main(int argc, char **argv)                                                                                      /* Keep the host tutorial C89-compatible. */
{                                                                                                                    /* Begin native device and context ownership. */
    FVM_Config config;                                                                                               /* Initialize the runtime configuration before use. */
    FVM_Runtime *runtime = 0;                                                                                        /* Track partial runtime creation. */
    FVM_Context *context = 0;                                                                                        /* Own the registered playback channel. */
    FVM_AlsaOutput *device = 0;                                                                                      /* Own the native ALSA provider independently of the channel. */
    FVM_AudioChannel *channel = 0;                                                                                   /* Borrow the context registration until successful close. */
    FVM_MediaOutput output;                                                                                          /* Receive actual ALSA write/drop callbacks. */
    FVM_MediaFrame frame;                                                                                            /* Describe the synchronously borrowed mono S16 buffer. */
    short samples[TUTORIAL_ALSA_SAMPLES];                                                                            /* Own generated native-endian PCM until write returns. */
    const unsigned char stop[] = {0x09, 0x00};                                                                       /* Encode AVM1 StopSounds followed by End. */
    char answer[8];                                                                                                  /* Consume one explicit Enter response without launching any hidden worker. */
    unsigned int i;                                                                                                  /* Generate each PCM sample in ordinary host code. */
    int exit_code = 1;                                                                                               /* Report success only after playback submission and stopping complete. */
    if (argc != 2)                                                                                                   /* Do not select a default device without the caller's choice. */
    {                                                                                                                /* Explain the optional native feature before opening anything. */
        printf("Usage: %s ALSA_DEVICE\nExplicitly plays a short tone; requires FVM_ENABLE_LINUX_AUDIO.\n", argv[0]); /* Describe the audible side effect. */
        return 0;                                                                                                    /* Keep no-argument runs free of device operations. */
    } /* Finish the explicit-device gate. */
    if (!checked(fvm_audio_linux_alsa_open(argv[1], TUTORIAL_ALSA_RATE, 1, TUTORIAL_ALSA_LATENCY_US, &device, &output), "alsa_open")) /* Open the caller-selected native endpoint. */
    {                                                                                                                                 /* Begin conditional branch: Open the caller-selected native endpoint. */
        goto cleanup;                                                                                                                 /* Default or foreign builds return UNSUPPORTED without touching devices. */
    } /* End conditional branch: Open the caller-selected native endpoint. */
    if (!checked(fvm_config_init(&config), "config_init")) /* Initialize the embedding configuration. */
    {                                                      /* Begin conditional branch: Initialize the embedding configuration. */
        goto cleanup;                                      /* Keep native provider ownership available for cleanup. */
    } /* End conditional branch: Initialize the embedding configuration. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Create the runtime. */
    {                                                                      /* Begin conditional branch: Create the runtime. */
        goto cleanup;                                                      /* Release the native device even after partial VM setup. */
    } /* End conditional branch: Create the runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context_create")) /* Create the playback owner context. */
    {                                                                      /* Begin conditional branch: Create the playback owner context. */
        goto cleanup;                                                      /* Preserve the required runtime/context destruction order. */
    } /* End conditional branch: Create the playback owner context. */
    if (!checked(fvm_audio_channel_create(context, &output, &channel), "channel_create")) /* Register copied ALSA callbacks. */
    {                                                                                     /* Begin conditional branch: Register copied ALSA callbacks. */
        goto cleanup;                                                                     /* The host still owns the native provider. */
    } /* End conditional branch: Register copied ALSA callbacks. */
    for (i = 0; i < TUTORIAL_ALSA_SAMPLES; ++i)                                                                         /* Fill a single explicitly requested demonstration frame. */
    {                                                                                                                   /* Begin loop body: Fill a single explicitly requested demonstration frame. */
        samples[i] = (short)((i / TUTORIAL_ALSA_HALF_PERIOD) % 2 ? TUTORIAL_ALSA_AMPLITUDE : -TUTORIAL_ALSA_AMPLITUDE); /* Generate a bounded square-wave sample. */
    } /* End loop body: Fill a single explicitly requested demonstration frame. */
    memset(&frame, 0, sizeof(frame));                                        /* Clear fields which do not describe audio. */
    frame.kind = FVM_MEDIA_AUDIO;                                            /* Select the documented signed-16 PCM representation. */
    frame.data = (const unsigned char *)samples;                             /* Borrow the local buffer only during write. */
    frame.size = sizeof(samples);                                            /* Publish the complete accessible byte range. */
    frame.sample_rate = TUTORIAL_ALSA_RATE;                                  /* Match the rate configured at native open. */
    frame.channels = 1;                                                      /* Match the mono endpoint configuration. */
    frame.samples = TUTORIAL_ALSA_SAMPLES;                                   /* Count sample frames rather than bytes. */
    if (!checked(fvm_audio_channel_write(channel, &frame), "channel_write")) /* Submit and start through real nonblocking ALSA calls. */
    {                                                                        /* Begin conditional branch: Submit and start through real nonblocking ALSA calls. */
        goto cleanup;                                                        /* A failed complete-frame write drops pending PCM before returning. */
    } /* End conditional branch: Submit and start through real nonblocking ALSA calls. */
    printf("Submitted a short tone. Press Enter to stop/discard the endpoint queue.\n"); /* Keep stopping under explicit user control. */
    if (!fgets(answer, sizeof(answer), stdin))                                           /* Wait only in the tutorial host, never inside a library callback. */
    {                                                                                    /* Begin conditional branch: Wait only in the tutorial host, never inside a library callback. */
        printf("Input ended; stopping the endpoint now.\n");                             /* Treat redirected input completion as a stop request. */
    } /* End conditional branch: Wait only in the tutorial host, never inside a library callback. */
    if (!checked(fvm_avm1_execute(context, stop, sizeof(stop)), "StopSounds")) /* Drop every active registered output in this context. */
    {                                                                          /* Begin conditional branch: Drop every active registered output in this context. */
        goto cleanup;                                                          /* Keep provider lifetime valid through terminal context cleanup. */
    } /* End conditional branch: Drop every active registered output in this context. */
    if (!checked(fvm_audio_channel_close(channel), "channel_close")) /* Unregister before releasing provider userdata. */
    {                                                                /* Begin conditional branch: Unregister before releasing provider userdata. */
        goto cleanup;                                                /* A failed close leaves the registration owned by context teardown. */
    } /* End conditional branch: Unregister before releasing provider userdata. */
    channel = 0;                      /* Discard the successfully closed registration handle. */
    exit_code = 0;                    /* Native submission and VM-driven stopping both completed. */
cleanup:                              /* Preserve provider userdata until every possible channel callback is finished. */
    if (context)                      /* Destroy only a successfully created channel owner. */
    {                                 /* Begin conditional branch: Destroy only a successfully created channel owner. */
        fvm_context_destroy(context); /* Stop remaining active channels before releasing their wrappers. */
    } /* End conditional branch: Destroy only a successfully created channel owner. */
    if (runtime)                      /* Runtime outlives its context. */
    {                                 /* Begin conditional branch: Runtime outlives its context. */
        fvm_runtime_destroy(runtime); /* Release runtime resources after channel callbacks are gone. */
    } /* End conditional branch: Runtime outlives its context. */
    if (!checked(fvm_audio_linux_alsa_close(device), "alsa_close")) /* Close the host-owned device last; null is accepted. */
    {                                                               /* Begin conditional branch: Close the host-owned device last; null is accepted. */
        exit_code = 1;                                              /* Preserve an actual native close failure in the exit status. */
    } /* End conditional branch: Close the host-owned device last; null is accepted. */
    return exit_code; /* Return normal status without assertions or abort. */
} /* Finish explicit native playback demonstration. */

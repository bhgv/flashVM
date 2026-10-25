/* Tutorial: explicitly authorize/open real microphone recognition and poll actual transcripts. */
#if !defined(_WIN32)                            /* Enable POSIX timer declarations only on POSIX targets. */
#define _POSIX_C_SOURCE 200809L                 /* Keep the tutorial's C89 syntax while using the OS timer API. */
#endif                                          /* Finish timer feature selection. */
#include "flashvm/speech.h"                     /* Import explicit recognition sessions and provider configuration. */
#include <stdio.h>                              /* Report actual recognized text and ordinary setup failures. */
#include <stdlib.h>                             /* Allocate exactly the transcript size returned by query mode. */
#include <string.h>                             /* Recognize explicit platform/model/device arguments. */
#ifdef _WIN32                                   /* Use the actual Windows host message pump in this standalone console tutorial. */
#include <windows.h>                            /* Dispatch STA messages and bound polling with Sleep. */
#elif defined(__APPLE__)                        /* Let actual Speech/AVFoundation callbacks run on the main loop. */
#include <CoreFoundation/CoreFoundation.h>      /* Run the real native event loop between polls. */
#else                                           /* Use an actual POSIX timer on Linux rather than spinning. */
#include <time.h>                               /* Declare timespec and nanosleep. */
#endif                                          /* Finish target-specific scheduler includes. */
#define TUTORIAL_STT_TICKS 1500U                /* Limit a manual capture run to approximately thirty seconds. */
#define TUTORIAL_STT_TICK_MILLISECONDS 20       /* Native scheduler delay between polls. */
#define TUTORIAL_STT_TICK_NANOSECONDS 20000000L /* POSIX duration matching the polling tick. */


/* Let native service callbacks run without adding a hidden thread to the speech library. */
static void host_tick(void)                           /* This scheduling policy belongs to the example host, not the speech API. */
{                                                     /* Begin one platform event-loop/scheduling tick. */
#ifdef _WIN32                                         /* Pump the standalone host's COM/UI apartment. */
    MSG message;                                      /* Receive queued native Windows messages. */
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) /* Keep native STA service notifications moving. */
    {                                                 /* Dispatch normal host messages while recognition remains active. */
        TranslateMessage(&message);                   /* Preserve normal keyboard translation in this standalone host. */
        DispatchMessage(&message);                    /* Invoke real window/COM message handling. */
    } /* Finish available native message dispatch. */
    Sleep(TUTORIAL_STT_TICK_MILLISECONDS); /* Avoid a busy polling loop during manual microphone use. */
#elif defined(__APPLE__)                   /* Run the actual main loop required by the Apple recognition host. */
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, TUTORIAL_STT_TICK_MILLISECONDS / 1000.0, /* Allow native service callbacks and timers to run. */
                       (Boolean)1);                                                    /* Allow native service callbacks and timers to run. */
#else                                      /* Wait through the real POSIX scheduler between bounded reads. */
    struct timespec delay;                         /* Describe one short native scheduling interval. */
    delay.tv_sec = 0;                              /* This tick is shorter than a whole second. */
    delay.tv_nsec = TUTORIAL_STT_TICK_NANOSECONDS; /* Use the explicit nanosecond tick constant. */
    (void)nanosleep(&delay, 0);                    /* An interrupt merely advances the next bounded polling iteration. */
#endif                                     /* Finish native host scheduling. */
} /* Finish one host tick. */


/* Capture only after the user explicitly requests system recognition or a Linux model/device. */
int main(int argc,                                     /* Permission acquisition is documented and remains the host application's responsibility. */
         char **argv)                                  /* Permission acquisition is documented and remains the host application's responsibility. */
{                                                      /* Begin explicit native recognition. */
    FVM_SttBackend backend;                            /* Receive real recognizer callbacks without opening the microphone yet. */
    FVM_LinuxSttConfig linux_config;                   /* Keep explicit model/device strings alive through the whole session. */
    FVM_SttSession *session = 0;                       /* Own actual microphone/recognizer resources until close. */
    FVM_Result result;                                 /* Track native setup, query/copy and service errors. */
    unsigned int tick;                                 /* Bound the manual listening loop. */
    int got_final = 0;                                 /* Distinguish a final actual transcript from a timeout/no-match run. */
    if (argc == 2 && strcmp(argv[1], "--system") == 0) /* Select real Windows/macOS services only. */
    {                                                  /* Begin conditional branch: Select real Windows/macOS services only. */
        result = fvm_stt_system_backend(&backend);     /* Select real Windows/macOS services only. */
    } /* End conditional branch: Select real Windows/macOS services only. */
    else if (argc == 4 &&                                /* Require an installed offline model and explicit ALSA capture name. */
             strcmp(argv[1], "--linux") == 0)            /* Require an installed offline model and explicit ALSA capture name. */
    {                                                    /* Bind actual Linux libraries with persistent host configuration. */
        linux_config.struct_size = sizeof(linux_config); /* Identify the exact public configuration layout. */
        linux_config.model_path = argv[2];               /* Borrow the actual installed Vosk model directory. */
        linux_config.alsa_device = argv[3];              /* Borrow the explicit microphone device, not a guessed path. */
        result = fvm_stt_linux_backend(&linux_config,    /* Report UNSUPPORTED if the real static SDK was not enabled. */
                                       &backend);        /* Report UNSUPPORTED if the real static SDK was not enabled. */
    } /* Finish explicit Linux recognizer selection. */
    else                                                                                          /* Keep automatic no-argument execution free of microphone access. */
    {                                                                                             /* Explain required manual setup before recording. */
        puts(                                                                                     /* Name native-service and model/device preparation. */
             "Usage: speech_stt --system OR --linux /installed/vosk-model explicit-alsa-device"); /* Name native-service and model/device preparation. */
        puts(                                                                                     /* Preserve explicit host authorization before capture. */
             "Grant microphone/speech permissions first; see this example's per-OS README.");     /* Preserve explicit host authorization before capture. */
        return argc == 1 ? 0 : 1;                                                                 /* The smoke run never opens a recognizer or microphone. */
    } /* Finish usage handling. */
    if (result != FVM_OK)                                             /* Preserve absent models/SDKs/system providers honestly. */
    {                                                                 /* Report setup failure without assertions. */
        fprintf(stderr, "Recognizer unavailable: %d\n", (int)result); /* Describe the actual backend result. */
        return 1;                                                     /* No native microphone state exists yet. */
    } /* Finish provider selection failure. */
    result = fvm_stt_open(&backend, &session);                         /* Explicitly activate the actual configured microphone recognizer. */
    if (result != FVM_OK)                                              /* Preserve native permission/device/engine startup failures. */
    {                                                                  /* Do not claim listening without successful capture setup. */
        fprintf(stderr, "Recognition open failed: %d\n", (int)result); /* Report the actual native startup result. */
        return 1;                                                      /* Failed open cleaned its partial resources. */
    } /* Finish native capture failure handling. */
    puts(                                                                    /* Describe the explicit manual microphone lifecycle. */
         "Listening for one final transcript, up to about thirty seconds."); /* Describe the explicit manual microphone lifecycle. */
    for (tick = 0; tick < TUTORIAL_STT_TICKS; ++tick)                        /* Keep capture bounded under the host's own scheduling policy. */
    {                                                                        /* Begin one actual recognition polling step. */
        size_t required = 0;                                                 /* Receive exact transcript bytes including NUL. */
        int final = 0;                                                       /* Receive final/partial state paired with the transcript. */
        char *text;                                                          /* Own a sized copy independently of provider storage. */
        result = fvm_stt_poll(session, 0, 0, &required, &final);             /* Query without consuming actual recognized text. */
        if (result == FVM_NOT_FOUND)                                         /* Let real service/audio work progress when no transcript is ready. */
        {                                                                    /* Yield through the native host scheduler. */
            host_tick();                                                     /* Avoid a hidden polling thread or CPU spin. */
            continue;                                                        /* Poll the same live recognizer on the next tick. */
        } /* Finish empty-queue scheduling. */
        if (result != FVM_OK) /* Preserve actual recognizer/capture failure. */
        {                     /* Begin conditional branch: Preserve actual recognizer/capture failure. */
            break;            /* Preserve actual recognizer/capture failure. */
        } /* End conditional branch: Preserve actual recognizer/capture failure. */
        text = (char *)malloc(required); /* Allocate precisely the stable queried transcript size. */
        if (!text)                       /* Release native capture later through the shared cleanup path. */
        {                                /* Preserve ordinary allocation errors. */
            result = FVM_OUT_OF_MEMORY;  /* Do not pretend transcript copying succeeded. */
            break;                       /* Leave the bounded polling loop for deterministic cleanup. */
        } /* Finish host-allocation failure handling. */
        result =                                                      /* Copy the same retained actual transcript. */
            fvm_stt_poll(session, text, required, &required, &final); /* Copy the same retained actual transcript. */
        if (result == FVM_OK)                                         /* Display actual native recognition output only. */
        {                                                             /* Begin conditional branch: Display actual native recognition output only. */
            printf("%s: %s\n", final ? "final" : "partial", text);    /* Display actual native recognition output only. */
        } /* End conditional branch: Display actual native recognition output only. */
        free(text);           /* Release host-owned text after printing, independent of the recognizer. */
        if (result != FVM_OK) /* Preserve native copy/provider errors. */
        {                     /* Begin conditional branch: Preserve native copy/provider errors. */
            break;            /* Preserve native copy/provider errors. */
        } /* End conditional branch: Preserve native copy/provider errors. */
        if (final)         /* End the demonstration after a real final recognition result. */
        {                  /* Do not silently restart capture for another utterance. */
            got_final = 1; /* Record actual final-transcript success. */
            break;         /* Close native capture through the shared cleanup path. */
        } /* Finish final-transcript handling. */
        host_tick(); /* Let native queues and microphone processing progress between partial results. */
    } /* Finish bounded actual recognition. */
    fvm_stt_close(session);                                      /* Stop microphone capture and release native recognition/model ownership. */
    if (!got_final)                                              /* No-match, timeout and real failure are useful outcomes, not fabricated text. */
    {                                                            /* Report actual manual-run status. */
        fprintf(stderr, "No final transcript; last status=%d\n", /* Preserve the native/empty-queue result. */
                (int)result);                                    /* Preserve the native/empty-queue result. */
        return 1;                                                /* Do not classify a no-result run as recognition success. */
    } /* Finish manual recognition outcome reporting. */
    return 0; /* A real final transcript was copied and all capture resources were released. */
} /* Finish the real STT tutorial. */

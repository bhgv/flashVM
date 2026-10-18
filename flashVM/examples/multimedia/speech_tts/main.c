/* Tutorial: prepare and invoke real native TTS, then explicitly stop/release the voice. */
#include "flashvm/speech.h" /* Import the replaceable system speech API. */
#include <stdio.h> /* Report setup errors and let the user finish listening before cleanup. */
#include <string.h> /* Recognize explicit system or Linux-daemon selection. */
/* Speak only after an explicit source/endpoint choice; default smoke execution is silent. */
int main(int argc, char **argv) /* Windows/macOS use --system; Linux uses --linux socket text. */
{ /* Begin explicit native speech use. */
    FVM_SpeechUnixEndpoint endpoint; /* Keep explicit abstract address configuration alive through session opening. */
    FVM_TtsBackend backend; /* Receive real native callbacks without activating playback yet. */
    FVM_TtsSession *session = 0; /* Own the native voice/service connection until common close. */
    FVM_Result result; /* Preserve setup, synthesis and cancellation failures normally. */
    const char *text; /* Borrow the explicit UTF-8 command-line text. */
    if (argc == 3 && strcmp(argv[1], "--system") == 0) /* Select the actual Windows/macOS provider. */
    { /* Initialize only the current target's real native voice callbacks. */
        result = fvm_tts_system_backend(&backend); /* Do not fall back to a fake synthesizer. */
        text = argv[2]; /* Borrow the requested plain speech text. */
    } /* Finish system provider selection. */
    else if (argc == 4 && /* Require an explicitly prepared Speech Dispatcher socket. */
             strcmp(argv[1], "--linux") == 0) /* Require an explicitly prepared Speech Dispatcher socket. */
    { /* Bind the real local daemon selected by the host. */
        result = fvm_tts_linux_backend(argv[2], &backend); /* No daemon is installed, started or discovered here. */
        text = argv[3]; /* Borrow the requested text for the speak call. */
    } /* Finish explicit Linux provider selection. */
    else if (argc == 4 && /* Select an abstract Linux/Android SSIP name explicitly. */
             strcmp(argv[1], "--abstract") == 0) /* Select an abstract Linux/Android SSIP name explicitly. */
    { /* Represent the name separately from its native leading namespace NUL. */
        endpoint.struct_size = sizeof(endpoint); /* Identify the binary-safe Unix endpoint ABI. */
        endpoint.name_space = /* Request the actual abstract namespace, not a filesystem path. */
            FVM_SPEECH_UNIX_ABSTRACT; /* Request the actual abstract namespace, not a filesystem path. */
        endpoint.name = argv[2]; /* Borrow printable CLI name bytes without a leading NUL. */
        endpoint.length = strlen( /* CLI strings are printable; native callers may supply embedded-NUL bytes with an explicit length. */
            argv[2]); /* CLI strings are printable; native callers may supply embedded-NUL bytes with an explicit length. */
        result = fvm_tts_unix_backend(&endpoint, /* Bind the explicit real SSIP endpoint without connecting yet. */
                                      &backend); /* Bind the explicit real SSIP endpoint without connecting yet. */
        text = argv[3]; /* Borrow plain Unicode speech text separately from the endpoint name. */
    } /* Finish explicit abstract endpoint selection. */
    else /* Keep no-argument execution harmless and explain platform preparation. */
    { /* Print the manual invocation contract. */
        puts(/* Name real provider and endpoint requirements. */
             "Usage: speech_tts --system text OR --linux /explicit/speechd/socket text OR --abstract service-name " /* Name real provider and endpoint requirements. */
             "text"); /* Name real provider and endpoint requirements. */
        puts(/* Link the required preparation concept to the tutorial documentation. */
             "Prepare the native voice/service first; see this example's README for each OS."); /* Link the required preparation concept to the tutorial documentation. */
        return argc == 1 ? 0 : 1; /* No-argument CTest execution never speaks or connects to a daemon. */
    } /* Finish usage handling. */
    if (result != FVM_OK) /* Explain absent providers and incompatible setup normally. */
    { /* Preserve ordinary error handling without assertions. */
        fprintf(stderr, "Backend unavailable: %d\n", (int)result); /* Report the actual public result category. */
        return 1; /* No native voice or service connection has opened. */
    } /* Finish unavailable-provider handling. */
    result = fvm_tts_open(&backend, &session); /* Explicitly open the actual system voice or named daemon. */
    if (result != FVM_OK) /* Preserve installed-engine/service/permission failures. */
    { /* No successful session exists to release. */
        fprintf(stderr, "Voice open failed: %d\n", (int)result); /* Describe actual native setup failure. */
        return 1; /* Do not claim audible speech without an open provider. */
    } /* Finish native open failure handling. */
    result = fvm_tts_speak(session, text); /* Validate Unicode and enqueue real synthesis/playback. */
    if (result == FVM_OK) /* Keep the session alive while asynchronous output plays. */
    { /* Let the user decide when playback is finished. */
        puts(/* Distinguish enqueue acceptance from completion. */
             "Speech accepted. Press Enter after listening; this will stop and close the voice."); /* Distinguish enqueue acceptance from completion. */
        fflush(stdout); /* Show the prompt before waiting on standard input. */
        (void)getchar(); /* Wait only during an explicit manual run, never the smoke invocation. */
        result = fvm_tts_stop(session); /* Explicitly cancel remaining speech on this provider only. */
    } /* Finish accepted-speech handling. */
    fvm_tts_close(session); /* Stop/release actual native resources even after speak/stop failure. */
    if (result != FVM_OK) /* Report native failure after deterministic cleanup. */
    { /* Keep a useful public diagnostic. */
        fprintf(stderr, "Speech/stop failed: %d\n", /* Preserve Unicode, readiness and real service errors. */
                (int)result); /* Preserve Unicode, readiness and real service errors. */
        return 1; /* Do not present a failed real operation as success. */
    } /* Finish native error reporting. */
    return 0; /* The selected native voice/service has been released. */
} /* Finish the real TTS tutorial. */

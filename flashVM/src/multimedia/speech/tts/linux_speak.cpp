/* OS: Linux/Android. Queue actual Speech Dispatcher playback with safe SSIP text framing. */
#include "linux_ssip.hpp" /* Import real connection and protocol helpers. */
#if defined(__linux__)    /* Isolate native SSIP implementation. */
#include <new>            /* Allocate escaped text without STL or exceptions. */
#include <string.h>       /* Measure the already bounded common-layer text. */


/* Normalize newlines and dot-stuff each line so speech text cannot become SSIP commands. */
FVM_Result fvm_linux_tts_speak(void *opaque, const char *text) /* Accept text only through the SPEAK data mode. */
{                                                              /* Begin actual daemon speech submission. */
    FVM_LinuxTts *state = static_cast<FVM_LinuxTts *>(opaque); /* Recover the owned speech-service connection. */
    size_t length = strlen(text);                              /* The common layer bounds accepted payload size. */
    char *escaped =                                            /* Reserve newline expansion, dot-stuffing and final framing. */
        new (std::nothrow) char[length * 2 + 5];               /* Reserve newline expansion, dot-stuffing and final framing. */
    if (!escaped)                                              /* Do not enter server data mode without complete local storage. */
    {                                                          /* Begin conditional branch: Do not enter server data mode without complete local storage. */
        return FVM_OUT_OF_MEMORY;                              /* Do not enter server data mode without complete local storage. */
    } /* End conditional branch: Do not enter server data mode without complete local storage. */
    size_t used = 0;                                   /* Track initialized escaped bytes. */
    bool start = true;                                 /* Dot-stuff at the beginning of each text line. */
    for (size_t i = 0; i < length; ++i)                /* Transform every bounded UTF-8 byte without interpreting markup. */
    {                                                  /* Begin one text-framing step. */
        char value = text[i];                          /* Preserve ordinary UTF-8 bytes unchanged. */
        if (value == '\r')                             /* Normalize CR and CRLF into one protocol newline. */
        {                                              /* Collapse an existing CRLF pair. */
            if (i + 1 < length && text[i + 1] == '\n') /* Consume a paired LF only once. */
            {                                          /* Begin conditional branch: Consume a paired LF only once. */
                ++i;                                   /* Consume a paired LF only once. */
            } /* End conditional branch: Consume a paired LF only once. */
            value = '\n'; /* Represent the normalized line ending internally. */
        } /* Finish CR normalization. */
        if (start && value == '.') /* Escape protocol-significant leading dots. */
        {                          /* Begin conditional branch: Escape protocol-significant leading dots. */
            escaped[used++] = '.'; /* Escape protocol-significant leading dots. */
        } /* End conditional branch: Escape protocol-significant leading dots. */
        if (value == '\n')          /* Emit canonical SSIP CRLF line endings. */
        {                           /* Begin conditional branch: Emit canonical SSIP CRLF line endings. */
            escaped[used++] = '\r'; /* Emit canonical SSIP CRLF line endings. */
        } /* End conditional branch: Emit canonical SSIP CRLF line endings. */
        escaped[used++] = value; /* Append the original byte or normalized newline. */
        start = value == '\n';   /* Track the next line's framing boundary. */
    } /* Finish bounded text transformation. */
    if (!start)                 /* Ensure the terminator begins on its own line. */
    {                           /* Finish an unterminated final text line. */
        escaped[used++] = '\r'; /* Append the protocol carriage return. */
        escaped[used++] = '\n'; /* Append the protocol newline. */
    } /* Finish final-line normalization. */
    escaped[used++] = '.';                                                                                        /* Mark the end of the SPEAK data block. */
    escaped[used++] = '\r';                                                                                       /* Finish the protocol dot line. */
    escaped[used++] = '\n';                                                                                       /* Complete the protocol message terminator. */
    const char command[] = "SPEAK\r\n";                                                                           /* Use a fixed command, never host text as protocol syntax. */
    bool success =                                                                                                /* Queue actual synthesis and verify service acceptance. */
        fvm_ssip_send(state, command, sizeof(command) - 1) && fvm_ssip_reply(state) == FVM_SSIP_RECEIVING_DATA && /* Queue actual synthesis and verify service acceptance. */
        fvm_ssip_send(state, escaped, used) &&                                                                    /* Queue actual synthesis and verify service acceptance. */
        fvm_ssip_reply(state) == FVM_SSIP_MESSAGE_QUEUED;                                                         /* Queue actual synthesis and verify service acceptance. */
    delete[] escaped;                                                                                             /* The daemon has consumed all submitted text bytes. */
    if (!success)                                                                                                 /* Close/reopen after any uncertain protocol state. */
    {                                                                                                             /* Begin conditional branch: Close/reopen after any uncertain protocol state. */
        state->failed = true;                                                                                     /* Close/reopen after any uncertain protocol state. */
    } /* End conditional branch: Close/reopen after any uncertain protocol state. */
    return success ? FVM_OK : FVM_ERROR; /* Accepted work is not an audible-completion event. */
} /* Finish real speech submission. */
#endif /* Finish Linux-only implementation. */

/* OS: Linux/Android. Parse bounded real SSIP multiline replies without storing daemon-controlled text. */
#include "linux_ssip.hpp" /* Share protocol codes and connection poison state. */
#if defined(__linux__)    /* Isolate native socket reads. */
#include <errno.h>        /* Retry interrupted receives. */
#include <sys/socket.h>   /* Declare the actual recv API. */


/* Read a numeric reply with consistent codes and a final space-delimited status line. */
int fvm_ssip_reply(FVM_LinuxTts *state)                              /* Return zero after malformed or incomplete protocol data. */
{                                                                    /* Begin bounded reply parsing. */
    int expected = 0;                                                /* Require every continuation to use the same reply code. */
    for (unsigned int count = 0; count < FVM_SSIP_REPLY_LINES_LIMIT; /* Bound daemon-controlled multiline work. */
         ++count)                                                    /* Bound daemon-controlled multiline work. */
    {                                                                /* Begin one status line. */
        char line[FVM_SSIP_REPLY_LINE_LIMIT];                        /* Keep a bounded native stack buffer. */
        size_t length = 0;                                           /* Track received bytes without trusting NUL termination. */
        while (length < sizeof(line))                                /* Read exactly through the next newline. */
        {                                                            /* Begin one native receive. */
            ssize_t got =                                            /* Use the real Unix socket with its configured timeout. */
                recv(state->fd, line + length, 1, 0);                /* Use the real Unix socket with its configured timeout. */
            if (got < 0 && errno == EINTR)                           /* Retry interruption without losing accumulated bytes. */
            {                                                        /* Begin conditional branch: Retry interruption without losing accumulated bytes. */
                continue;                                            /* Retry interruption without losing accumulated bytes. */
            } /* End conditional branch: Retry interruption without losing accumulated bytes. */
            if (got != 1) /* Handle timeout, EOF and hard receive errors together. */
            {             /* Begin conditional branch: Handle timeout, EOF and hard receive errors together. */
                break;    /* Handle timeout, EOF and hard receive errors together. */
            } /* End conditional branch: Handle timeout, EOF and hard receive errors together. */
            if (line[length++] == '\n') /* Stop before consuming a later protocol response. */
            {                           /* Begin conditional branch: Stop before consuming a later protocol response. */
                break;                  /* Stop before consuming a later protocol response. */
            } /* End conditional branch: Stop before consuming a later protocol response. */
        } /* Finish bounded line reception. */
        if (length < 5 || line[length - 1] != '\n' || line[0] < '0' || line[0] > '9' || line[1] < '0' || /* Require the documented SSIP status-line shape. */
            line[1] > '9' || line[2] < '0' || line[2] > '9' || (line[3] != ' ' && line[3] != '-'))       /* Require the documented SSIP status-line shape. */
        {                                                                                                /* Begin scope: Require the documented SSIP status-line shape. */
            break;                                                                                       /* Require the documented SSIP status-line shape. */
        } /* End scope: Require the documented SSIP status-line shape. */
        int code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + line[2] - /* Decode the protocol's three decimal status digits. */
                   '0';                                                     /* Decode the protocol's three decimal status digits. */
        if (expected && expected != code)                                   /* Reject mixed-code multiline replies. */
        {                                                                   /* Begin conditional branch: Reject mixed-code multiline replies. */
            break;                                                          /* Reject mixed-code multiline replies. */
        } /* End conditional branch: Reject mixed-code multiline replies. */
        expected = code;    /* Remember this reply's numeric code. */
        if (line[3] == ' ') /* Finish only at the explicit final status line. */
        {                   /* Begin conditional branch: Finish only at the explicit final status line. */
            return code;    /* Finish only at the explicit final status line. */
        } /* End conditional branch: Finish only at the explicit final status line. */
    } /* Finish bounded reply parsing. */
    state->failed = true; /* Do not reuse a truncated, timed-out or malformed stream. */
    return 0;             /* Preserve a simple failure signal for native callbacks. */
} /* Finish SSIP reply parsing. */
#endif /* Finish Linux-only implementation. */

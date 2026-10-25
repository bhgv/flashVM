/* OS: macOS. Poll the real Core Graphics global pointer location. */
#include "flashvm/multimedia.h"                      /* Import the independent host event view. */
#include <string.h>                                  /* Clear failed or unsupported results. */
#ifdef __APPLE__                                     /* Isolate Apple framework declarations. */
#include <ApplicationServices/ApplicationServices.h> /* Declare Core Graphics event and ownership APIs. */
#endif                                               /* Finish target-specific includes. */


/* Create and release one temporary Quartz event to inspect its current location. */
FVM_Result fvm_input_macos_pointer(FVM_DeviceEvent *out) /* Do not install event taps or retain host callbacks. */
{                                                        /* Begin global pointer polling. */
    if (!out)                                            /* Require writable event storage. */
    {                                                    /* Begin conditional branch: Require writable event storage. */
        return FVM_INVALID_ARGUMENT;                     /* Require writable event storage. */
    } /* End conditional branch: Require writable event storage. */
    memset(out, 0, sizeof(*out));        /* Clear all unsupported/failure fields. */
#ifdef __APPLE__                         /* Invoke real Core Graphics only on the Apple target. */
    CGEventRef event = CGEventCreate(0); /* Create a temporary current-state event. */
    if (!event)                          /* Preserve OS allocation/availability failure. */
    {                                    /* Begin conditional branch: Preserve OS allocation/availability failure. */
        return FVM_ERROR;                /* Preserve OS allocation/availability failure. */
    } /* End conditional branch: Preserve OS allocation/availability failure. */
    CGPoint point = CGEventGetLocation(event); /* Read coordinates in global display space. */
    CFRelease(event);                          /* Release the temporary owned Quartz object promptly. */
    out->kind = FVM_DEVICE_MOUSE;              /* Report a complete pointer position. */
    out->x = point.x;                          /* Preserve native display X without stage scaling. */
    out->y = point.y;                          /* Preserve native display Y without stage scaling. */
    return FVM_OK;                             /* Leave coordinate conversion and dispatch to the application. */
#else                                          /* Preserve the public ABI on other targets. */
    return FVM_UNSUPPORTED; /* Do not invent an Apple pointer position. */
#endif                                         /* Finish target selection. */
} /* Finish Core Graphics pointer polling. */

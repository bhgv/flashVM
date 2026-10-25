/* OS: Android NDK. Decode a borrowed native motion event, one pointer at a time. */
#include "flashvm/multimedia.h" /* Import portable event storage and size_t. */
#include <string.h>             /* Clear stale event results. */
#ifdef __ANDROID__              /* Isolate the real Android input SDK. */
#include <android/input.h>      /* Declare type-checked AInputEvent/AMotionEvent accessors. */
#endif                          /* Finish Android-only includes. */


/* Read a motion event without acquiring/releasing queue ownership. */
FVM_Result fvm_input_android_motion(const void *native, size_t index, /* Borrow the event for this synchronous call. */
                                    FVM_DeviceEvent *out)             /* Borrow the event for this synchronous call. */
{                                                                     /* Begin pointer-event extraction. */
    if (!out)                                                         /* Require writable output. */
    {                                                                 /* Begin conditional branch: Require writable output. */
        return FVM_INVALID_ARGUMENT;                                  /* Require writable output. */
    } /* End conditional branch: Require writable output. */
    memset(out, 0, sizeof(*out));    /* Clear all failure paths. */
    if (!native)                     /* Reject an absent borrowed event. */
    {                                /* Begin conditional branch: Reject an absent borrowed event. */
        return FVM_INVALID_ARGUMENT; /* Reject an absent borrowed event. */
    } /* End conditional branch: Reject an absent borrowed event. */
#ifdef __ANDROID__                                                       /* Decode only with the native Android ABI. */
    const AInputEvent *event = static_cast<const AInputEvent *>(native); /* Recover the caller's live NDK event. */
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)          /* Never call motion accessors on key events. */
    {                                                                    /* Begin conditional branch: Never call motion accessors on key events. */
        return FVM_INVALID_ARGUMENT;                                     /* Never call motion accessors on key events. */
    } /* End conditional branch: Never call motion accessors on key events. */
    if (index >= AMotionEvent_getPointerCount(event)) /* Validate the pointer index before all indexed accessors. */
    {                                                 /* Begin conditional branch: Validate the pointer index before all indexed accessors. */
        return FVM_INVALID_ARGUMENT;                  /* Validate the pointer index before all indexed accessors. */
    } /* End conditional branch: Validate the pointer index before all indexed accessors. */
    out->kind = FVM_DEVICE_TOUCH;                             /* Report one pointer sample, including mouse/stylus motion sources. */
    out->native_code =                                        /* Preserve action and packed changed-pointer index bits. */
        (unsigned int)AMotionEvent_getAction(event);          /* Preserve action and packed changed-pointer index bits. */
    out->x = AMotionEvent_getX(event, index);                 /* Preserve window-local X in native pixels. */
    out->y = AMotionEvent_getY(event, index);                 /* Preserve window-local Y in native pixels. */
    out->z =                                                  /* Preserve stable contact identity separately from array index. */
        AMotionEvent_getPointerId(event, index);              /* Preserve stable contact identity separately from array index. */
    out->seconds = (double)AMotionEvent_getEventTime(event) / /* Convert the native monotonic nanosecond timestamp. */
                   FVM_DEVICE_NANOSECONDS_PER_SECOND;         /* Convert the native monotonic nanosecond timestamp. */
    return FVM_OK;                                            /* The host still finishes and releases its queue event. */
#else                                                         /* Preserve the public signature on other targets. */
    (void)index;            /* No NDK event interpretation is attempted here. */
    return FVM_UNSUPPORTED; /* Require the host target's own event adapter. */
#endif                                                        /* Finish target selection. */
} /* Finish native Android motion extraction. */

/* OS: Windows. Poll physical virtual-key state through User32. */
#include "flashvm/multimedia.h" /* Import device-event categories. */
#include <string.h>             /* Clear failure results. */
#ifdef _WIN32                   /* Isolate Windows SDK declarations. */
#include <windows.h>            /* Declare GetAsyncKeyState and virtual-key handling. */
#endif                          /* Finish Windows-only includes. */
/* Report current down/up state; the unreliable historical low bit is ignored. */
FVM_Result                                  /* A zero OS result cannot distinguish up from inaccessible desktop. */
fvm_input_windows_key(unsigned int key,     /* A zero OS result cannot distinguish up from inaccessible desktop. */
                      FVM_DeviceEvent *out) /* A zero OS result cannot distinguish up from inaccessible desktop. */
{                                           /* Begin physical-key polling. */
    if (!out)                               /* Require result storage. */
    {                                       /* Begin conditional branch: Require result storage. */
        return FVM_INVALID_ARGUMENT;        /* Require result storage. */
    } /* End conditional branch: Require result storage. */
    memset(out, 0, sizeof(*out));        /* Clear stale key information. */
#ifdef _WIN32                            /* Query only the actual Windows desktop. */
    if (key > FVM_DEVICE_WINDOWS_VK_MAX) /* Restrict input to the documented virtual-key code space. */
    {                                    /* Begin conditional branch: Restrict input to the documented virtual-key code space. */
        return FVM_INVALID_ARGUMENT;     /* Restrict input to the documented virtual-key code space. */
    } /* End conditional branch: Restrict input to the documented virtual-key code space. */
    out->kind = FVM_DEVICE_KEY;                          /* Report physical key state rather than a queued transition. */
    out->native_code = key;                              /* Preserve the caller's virtual-key identifier. */
    out->x = GetAsyncKeyState((int)key) < 0 ? 1.0 : 0.0; /* Use only the current-state sign bit. */
    return FVM_OK;                                       /* Preserve the API's up/inaccessible-desktop ambiguity. */
#else                                                    /* Keep the symbol callable on non-Windows builds. */
    (void)key;              /* No native virtual-key interpretation exists on this target. */
    return FVM_UNSUPPORTED; /* Require a target-specific bridge instead. */
#endif                                                   /* Finish platform selection. */
} /* Finish Windows keyboard polling. */

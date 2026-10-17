/* OS: Windows. Poll real desktop pointer coordinates without sending script events. */
#include "flashvm/multimedia.h" /* Import the portable device event. */
#include <string.h> /* Clear unsupported or failed results. */
#ifdef _WIN32 /* Keep Windows headers off other targets. */
#include <windows.h> /* Declare the actual GetCursorPos API. */
#endif /* Finish target-specific includes. */
/* Return current screen coordinates; host code maps them into its stage/window. */
FVM_Result fvm_input_windows_pointer(FVM_DeviceEvent *out) /* Never create or own a window. */
{ /* Begin pointer polling. */
    if (!out) /* Require a writable result. */
        return FVM_INVALID_ARGUMENT; /* Require a writable result. */
    memset(out, 0, sizeof(*out)); /* Clear all fields before querying the OS. */
#ifdef _WIN32 /* Invoke User32 only on Windows. */
    POINT point; /* Receive global desktop pixel coordinates. */
    if (!GetCursorPos(&point)) /* Preserve inaccessible-desktop/API failure. */
        return FVM_ERROR; /* Preserve inaccessible-desktop/API failure. */
    out->kind = FVM_DEVICE_MOUSE; /* Identify a complete current pointer position. */
    out->x = point.x; /* Preserve global screen X. */
    out->y = point.y; /* Preserve global screen Y. */
    return FVM_OK; /* Leave VM event dispatch to the host. */
#else /* Keep cross-platform linkage predictable. */
    return FVM_UNSUPPORTED; /* Do not emulate a Windows desktop on another OS. */
#endif /* Finish platform selection. */
} /* Finish Windows pointer polling. */

/* OS: Linux evdev. The host opens a nonblocking descriptor and retains ownership. */
#include "flashvm/multimedia.h" /* Import the portable device event view. */
#include <string.h> /* Clear the caller's event deterministically. */
#if defined(__linux__) && !defined(__ANDROID__) /* Keep Linux kernel interfaces off other toolchains. */
#include <errno.h> /* Distinguish retryable reads from OS failures. */
#include <linux/input.h> /* Use the kernel's native input_event layout and event codes. */
#include <unistd.h> /* Call the real read system interface. */
#endif /* Finish Linux-only includes. */
/* Read one record; native coordinates, relative deltas, slots and contact IDs remain unnormalized.
 */
FVM_Result fvm_input_linux_evdev_read(int fd, /* Do not open, close or change the descriptor flags. */
                                      FVM_DeviceEvent *out) /* Do not open, close or change the descriptor flags. */
{ /* Begin native input polling. */
    if (!out) /* Require output storage. */
        return FVM_INVALID_ARGUMENT; /* Require output storage. */
    memset(out, 0, sizeof(*out)); /* Clear stale records on all failure paths. */
    if (fd < 0) /* Reject an invalid descriptor before the system call. */
        return FVM_INVALID_ARGUMENT; /* Reject an invalid descriptor before the system call. */
#if defined(__linux__) && !defined(__ANDROID__) /* Compile the actual Linux adapter only on Linux. */
    struct input_event event; /* Receive exactly one native kernel event. */
    ssize_t bytes = read(fd, &event, sizeof(event)); /* Read through the host-owned descriptor. */
    if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) /* Leave retry scheduling to the host. */
        return FVM_NOT_FOUND; /* Leave retry scheduling to the host. */
    if (bytes != sizeof(event)) /* Reject EOF, partial records and hard read errors. */
        return FVM_ERROR; /* Reject EOF, partial records and hard read errors. */
    if (event.type == EV_KEY) /* Preserve keyboard and mouse-button transitions. */
        out->kind = FVM_DEVICE_KEY; /* Preserve keyboard and mouse-button transitions. */
    else if (event.type == EV_REL) /* Preserve relative pointer/wheel deltas. */
        out->kind = FVM_DEVICE_MOUSE; /* Preserve relative pointer/wheel deltas. */
    else if (event.type == EV_ABS) /* Pass the declared arguments to the named bridge operation. */
        out->kind = event.code >= ABS_MT_SLOT && event.code <= ABS_MT_TOOL_Y /* Continue the same operation using the explicitly listed arguments. */
                        ? FVM_DEVICE_TOUCH /* Continue the same operation using the explicitly listed arguments. */
                        : FVM_DEVICE_MOUSE; /* Preserve multitouch fragments separately from
                                               absolute pointer axes. */
    else /* Continue the same operation using the explicitly listed arguments. */
        return FVM_NOT_FOUND; /* Ignore synchronization/configuration records, one record per call.
                               */
    out->native_code = event.code; /* Preserve the kernel key or axis identifier for host translation. */
    out->x = event.value; /* Preserve the native value, including repeat, slot and contact identifiers. */
    out->seconds = /* Continue the same operation using the explicitly listed arguments. */
        (double)event.time.tv_sec + /* Pass the declared arguments to the named bridge operation. */
        (double)event.time.tv_usec / FVM_DEVICE_MICROSECONDS_PER_SECOND; /* Convert the native event timestamp
                                                              without changing its clock domain. */
    return FVM_OK; /* Report one untranslated native record. */
#else /* Describe behavior on Windows, macOS and Android explicitly. */
    return FVM_UNSUPPORTED; /* Let those hosts install their own native queue callbacks. */
#endif /* Finish platform selection. */
} /* Finish Linux evdev polling. */

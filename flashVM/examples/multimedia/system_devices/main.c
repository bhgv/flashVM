/* Tutorial: adapt a host-owned nonblocking Linux evdev descriptor to the polling ABI. */
#include "flashvm/multimedia.h" /* Import the common device bridge and native evdev adapter. */
#include <stdio.h> /* Print native records and ordinary errors. */
#if defined(__linux__) && !defined(__ANDROID__) /* Restrict native device opening to Linux. */
#include <fcntl.h> /* Use the real nonblocking open flags. */
#include <unistd.h> /* Close the host-owned descriptor exactly once. */
#endif /* Finish platform-only includes. */
/* Forward one native device read; the descriptor belongs to main, not the bridge. */
static FVM_Result poll_device(void *opaque, FVM_DeviceEvent *event) /* Borrow descriptor storage synchronously. */
{ /* Begin the custom host callback. */
    int *fd = (int *)opaque; /* Recover the application's live descriptor variable. */
    return fvm_input_linux_evdev_read(*fd, event); /* Delegate to actual evdev system I/O. */
} /* Finish the custom device callback. */
/* Poll at most 64 native records; no device path or permission is guessed. */
int main(int argc, char **argv) /* Require the user to explicitly name a readable event device. */
{ /* Begin explicit device polling. */
    FVM_DeviceBridge bridge; /* Borrow a host queue through the uniform callback table. */
    FVM_DeviceEvent event; /* Receive one raw device event at a time. */
    FVM_Result result; /* Distinguish no event from real OS errors. */
    int fd; /* Own a Linux descriptor only after successful open. */
    unsigned int count; /* Bound polling without a hidden event thread. */
    if (argc != 2) /* Avoid opening devices during no-argument smoke execution. */
    { /* Describe the explicit permission-controlled path. */
        puts("Usage on Linux: system_devices /dev/input/eventN (read permission required)"); /* Do
                                                                                                not
                                                                                                change
                                                                                                permissions
                                                                                                or
                                                                                                discover
                                                                                                devices
                                                                                                automatically.
                                                                                              */
        return argc == 1 ? 0 : 1; /* Report malformed arguments normally. */
    } /* Finish usage handling. */
#if defined(__linux__) && !defined(__ANDROID__) /* Open only on a target with real evdev support.  \
                                                 */
    fd = open(argv[1], O_RDONLY | O_NONBLOCK); /* Borrow no external session and open only the named device. */
    if (fd < 0) /* Preserve actual device permission or availability failure. */
    { /* Report normal OS failure. */
        perror("Device open failed"); /* Print the real errno message. */
        return 1; /* No descriptor needs cleanup after failed open. */
    } /* Finish device-open error handling. */
#else /* Do not reinterpret Linux paths on other operating systems. */
    (void)argv; /* No native path is used by the unsupported branch. */
    fd = -1; /* Keep the unsupported path initialized for portable compilation. */
    puts("This tutorial requires Linux evdev; use native callbacks on your OS."); /* Identify the
                                                                                     actual platform
                                                                                     limitation. */
    return 1; /* Do not fabricate device input. */
#endif /* Finish platform-specific opening. */
    bridge.struct_size = sizeof(bridge); /* Identify the exact device callback layout. */
    bridge.userdata = &fd; /* Borrow the descriptor variable until polling finishes. */
    bridge.poll = poll_device; /* Install the custom system bridge callback. */
    result = FVM_OK; /* Start with no previously observed OS error. */
    for (count = 0; count < 64; ++count) /* Bound native polling without waiting for user input. */
    { /* Begin one host scheduling tick. */
        result = fvm_device_poll(&bridge, &event); /* Poll one raw record through the replaceable table. */
        if (result == FVM_NOT_FOUND) /* A filtered/no-event record is harmless; retry within the bounded tick. */
            continue; /* A filtered/no-event record is harmless; retry within the bounded tick. */
        if (result != FVM_OK) /* Stop on a real descriptor or backend failure. */
            break; /* Stop on a real descriptor or backend failure. */
        printf("kind=%d native_code=%u value=%g time=%g\n", event.kind, event.native_code, event.x, /* Preserve raw codes/units for host normalization. */
               event.seconds); /* Preserve raw codes/units for host normalization. */
    } /* Finish bounded polling. */
#if defined(__linux__) && !defined(__ANDROID__) /* Release only a descriptor opened on this platform. */
    if (close(fd) != 0) /* Report final native handle-close errors too. */
        result = FVM_ERROR; /* Report final native handle-close errors too. */
#endif /* Finish descriptor cleanup. */
    if (result != FVM_OK && result != FVM_NOT_FOUND) /* Report genuine native failures after cleanup. */
    { /* Keep ordinary tutorial error handling. */
        fprintf(stderr, "Poll failed: %d\n", (int)result); /* Describe the bridge result code. */
        return 1; /* Preserve failure to the host shell. */
    } /* Finish polling error reporting. */
    return 0; /* The application remains responsible for scaling and VM/script dispatch. */
} /* Finish main. */

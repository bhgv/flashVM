/* Direct Linux frontend: fbdev output, one evdev/mousedev pointer and optional V4L2 Y preview. */
#include "device.h" /* Validated native framebuffer and bounded MMAP camera ownership. */
#include <poll.h> /* Nonblocking input/camera readiness with bounded wait. */
#include <signal.h> /* SIGINT/SIGTERM request owner-thread cleanup only. */
#include <stdio.h> /* Explicit usage, native diagnostics and frame counters. */
#define FB_DEMO_POLL_MS 16 /* Coarse frame interval; not an audio/movie clock. */
/* Pointer state is native copied data, never managed by the VM collector. */
typedef struct PointerDevice { /* Only one input descriptor is selected per run. */
    int fd; /* Owned evdev or mousedev file descriptor, initialized to -1. */
    int mouse; /* Three-byte mousedev stream instead of evdev records. */
    double x, y; /* Tutorial coordinates clamped into its fixed framebuffer rectangle. */
    struct input_absinfo ax, ay; /* Optional calibrated legacy single-pointer ABS axes. */
    int dropping; /* Ignore damaged evdev records through the next SYN_REPORT. */
    int (*query)(int, unsigned long, void *); /* Native ioctl wrapper; replaceable only by decoder fixtures. */
    int absolute; /* Both ABS_X and ABS_Y queried successfully with nonempty ranges. */
    unsigned char packet[FB_DEMO_MOUSE_BYTES]; /* Preserve partial PS/2 packet reads. */
    unsigned int used; /* Number of packet bytes collected so far. */
    double wheel_legacy; /* Legacy vertical detents accumulated until SYN_REPORT. */
    double wheel_highres; /* High-resolution vertical counts accumulated for the same report. */
    int wheel_has_highres; /* Detect high-resolution report data in device-free decoder fixtures too. */
    int wheel_highres_available; /* Queried capability: ignore legacy detents even in separate native reports. */
} PointerDevice; /* Multi-touch slots and keyboard translation are intentionally separate work. */
/* Composed output owners outlive each synchronous frame callback. */
typedef struct LinuxHost { /* Main zeroes all fields before opening native resources. */
    FbDevice fb; /* Existing framebuffer mode and mapping. */
    VideoDevice video; /* Optional bounded camera queue and copied preview. */
    PointerDevice pointer; /* At most one source avoids duplicate evdev/mousedev delivery. */
} LinuxHost; /* No native worker calls FlashVM. */
static volatile sig_atomic_t stopping = 0; /* Signal-safe stop request; no VM work inside a handler. */
/* Convert a process termination request into ordinary owner-loop cleanup. */
static void request_stop(int number) { /* Handler may interrupt read/poll/ioctl. */
    (void)number; stopping = 1; /* Only async-signal-safe scalar assignment. */
} /* Descriptors and mappings remain owned by main. */
/* Submit a complete opaque RGBA frame into the already validated device rectangle. */
static FVM_Result linux_present(const FVM_RasterSurface *frame, void *data) { /* Synchronous borrowed sink. */
    LinuxHost *host = (LinuxHost *)data; /* Native mappings outlive this callback. */
    unsigned int x, y, byte, bytes = host->fb.mode.bits_per_pixel / CHAR_BIT; /* Validated packed format. */
    for (y = 0; y < frame->height; ++y) { /* Preserve native stride and virtual-page Y offset. */
        unsigned char *destination = host->fb.map + ((size_t)y + host->fb.mode.yoffset) * /* Checked span. */
            host->fb.fixed.line_length + (size_t)host->fb.mode.xoffset * bytes; /* Visible X origin. */
        for (x = 0; x < frame->width; ++x) { /* Convert only the tutorial rectangle. */
            const unsigned char *pixel = frame->pixels + y * frame->stride + x * FVM_HOST_RGBA_BYTES; /* RGBA. */
            unsigned int r = pixel[0], g = pixel[1], b = pixel[2]; /* Opaque source channels. */
            unsigned long packed; /* Native little-endian truecolor value. */
            if (host->video.frames && r == DEMO_IDLE_RGB && g == DEMO_IDLE_RGB && b == DEMO_IDLE_RGB) /* Background. */
                r = g = b = host->video.preview[y * DEMO_WIDTH + x]; /* Marker stays above raw-Y preview. */
            packed = fb_channel(r, host->fb.mode.red) | fb_channel(g, host->fb.mode.green) | /* Pack RGB. */
                fb_channel(b, host->fb.mode.blue) | fb_channel(DEMO_OPAQUE, host->fb.mode.transp); /* Opaque alpha. */
            for (byte = 0; byte < bytes; ++byte) /* Explicit little-endian byte order, no unaligned casts. */
                destination[x * bytes + byte] = (unsigned char)(packed >> (byte * CHAR_BIT)); /* Visible write. */
        } /* Source storage remains owned by the caller. */
    } /* No page flip, vsync or physical scanout completion is implied. */
    return FVM_OK; /* Layout was validated before mmap and never changed by this example. */
} /* fbdev mmap bus/device failures are outside the synchronous C ABI status mechanism. */
/* Clamp the native relative pointer and publish a single explicit VM transition. */
static int pointer_send(PointerDevice *pointer, Demo *demo, FVM_MouseEventType type) { /* Owner-thread only. */
    FVM_Affine identity = {1, 0, 0, 1, 0, 0}; /* Fixed top-left rectangle, one world pixel per fb pixel. */
    if (pointer->x < 0) pointer->x = 0; /* This device tutorial deliberately confines its pointer. */
    if (pointer->y < 0) pointer->y = 0; /* No negative framebuffer cursor positions. */
    if (pointer->x >= DEMO_WIDTH) pointer->x = DEMO_WIDTH - 1; /* Bound visible horizontal marker. */
    if (pointer->y >= DEMO_HEIGHT) pointer->y = DEMO_HEIGHT - 1; /* Bound vertical marker. */
    return demo_pointer(demo, type, &identity, pointer->x, pointer->y) == FVM_OK; /* Preserve real VM failure. */
} /* Coordinates are copied into the context, not retained by pointer. */
/* Query one copied device snapshot through the native descriptor, retrying interrupted ioctls. */
static int pointer_query(int fd, unsigned long request, void *output) /* No VM calls in this helper. */
{ /* The caller owns every fixed-size output buffer. */
    int result; /* Preserve errno from the last failed ioctl. */
    do { result = ioctl(fd, request, output); } while (result < 0 && errno == EINTR); /* Retry signal interruption. */
    return result; /* Nonnegative native results indicate a completed query. */
} /* No resource or output pointer is retained. */
/* Recover current buttons and calibrated absolute position after the ignored report boundary. */
static int pointer_resync(PointerDevice *pointer, Demo *demo) /* Relative motion cannot be reconstructed. */
{ /* Collect every required native query before publishing a recovered VM transition. */
    unsigned char keys[FB_DEMO_KEY_BITS_BYTES]; /* Kernel bitmap, zero unused tail bytes. */
    struct input_absinfo ax, ay; /* Stage new absolute values/calibration transactionally. */
    int held; /* Snapshot primary state; this is not a replay of lost presses. */
    int (*query)(int, unsigned long, void *) = pointer->query ? pointer->query : pointer_query; /* Real native default. */
    memset(keys, 0, sizeof(keys)); /* ioctl may return fewer bytes than requested. */
    if (query(pointer->fd, EVIOCGKEY(sizeof(keys)), keys) < 0) return 0; /* Disconnected/unsupported device fails closed. */
    if (pointer->absolute) /* Relative devices keep the last accepted VM position. */
    { /* A changed calibration is valid only when both queried ranges remain nonempty. */
        if (query(pointer->fd, EVIOCGABS(ABS_X), &ax) < 0 || /* Complete both native coordinate queries. */
            query(pointer->fd, EVIOCGABS(ABS_Y), &ay) < 0) return 0; /* No partial VM publication. */
        if (ax.maximum <= ax.minimum || ay.maximum <= ay.minimum) { errno = EINVAL; return 0; } /* No zero divisor. */
        pointer->ax = ax; pointer->ay = ay; /* Commit copied calibration after full validation. */
        pointer->x = ((double)ax.value - ax.minimum) * (DEMO_WIDTH - 1) / /* Convert without signed subtraction overflow. */
            ((double)ax.maximum - ax.minimum); /* Fixed world rectangle. */
        pointer->y = ((double)ay.value - ay.minimum) * (DEMO_HEIGHT - 1) / /* Same scale as ordinary ABS reports. */
            ((double)ay.maximum - ay.minimum); /* pointer_send clamps out-of-range device values. */
    } /* Lost relative deltas and wheel events are deliberately not guessed. */
    held = ((keys[BTN_LEFT / CHAR_BIT] & (1U << (BTN_LEFT % CHAR_BIT))) != 0) || /* Mouse primary button. */
           ((keys[BTN_TOUCH / CHAR_BIT] & (1U << (BTN_TOUCH % CHAR_BIT))) != 0); /* Legacy single touch contact. */
    if (!pointer_send(pointer, demo, held ? FVM_MOUSE_EVENT_DOWN : FVM_MOUSE_EVENT_MOVE)) return 0; /* Explicit resume policy. */
    pointer->dropping = 0; /* Resume ordinary records only after VM acceptance. */
    return 1; /* A held snapshot begins a new contact after the cancellation at loss. */
} /* No claim that snapshot ioctls constitute an atomic hardware transaction. */
/* Consume one complete legacy evdev record, preserving native record order. */
static int evdev_event(PointerDevice *pointer, Demo *demo, const struct input_event *event) { /* Borrow record. */
    if (event->type == EV_SYN && event->code == SYN_DROPPED) { /* Kernel queue lost synchronization. */
        pointer->dropping = 1; /* Repeated loss markers keep the recovery boundary pending. */
        pointer->wheel_legacy = pointer->wheel_highres = 0; pointer->wheel_has_highres = 0; /* Discard partial wheel packet. */
        demo_cancel(demo); /* End a previous drag at its last accepted VM coordinate. */
        if (demo->status != FVM_OK) return 0; /* Handler failure cannot be hidden by recovery. */
        demo->status = fvm_input_get_pointer(demo->context, &pointer->x, &pointer->y); /* Discard unpublished REL motion. */
        return demo->status == FVM_OK; /* Keep reading until the mandatory synchronization boundary. */
    } /* Never interpret subsequent stale records as a continuous drag. */
    if (pointer->dropping) /* Kernel protocol requires ignoring through and including the next report. */
    { /* No stale button, axis or wheel event reaches ordinary decoding. */
        if (event->type == EV_SYN && event->code == SYN_REPORT) return pointer_resync(pointer, demo); /* Query current state. */
        return 1; /* Other records are intentionally discarded, including SYN_MT_REPORT. */
    } /* The first event after a successful snapshot resumes normal delivery. */
    if (event->type == EV_REL && event->code == REL_X) pointer->x += event->value; /* Relative X motion. */
    if (event->type == EV_REL && event->code == REL_Y) pointer->y += event->value; /* Relative Y motion. */
    if (event->type == EV_REL && event->code == REL_WHEEL) pointer->wheel_legacy += event->value; /* Detents. */
#ifdef REL_WHEEL_HI_RES /* Preserve compatibility with kernel SDKs predating high-resolution wheel codes. */
    if (event->type == EV_REL && event->code == REL_WHEEL_HI_RES) { /* Same packet may also carry legacy detents. */
        pointer->wheel_highres += event->value; pointer->wheel_has_highres = 1; /* Keep fractional detent counts. */
    } /* Do not dispatch legacy and high-resolution movement twice. */
#endif /* Older kernel headers retain the documented legacy path. */
    if (event->type == EV_ABS && pointer->absolute && event->code == ABS_X) /* Legacy calibrated absolute X. */
        pointer->x = ((double)event->value - pointer->ax.minimum) * (DEMO_WIDTH - 1) / /* Avoid signed overflow. */
            ((double)pointer->ax.maximum - pointer->ax.minimum); /* Full calibrated device range. */
    if (event->type == EV_ABS && pointer->absolute && event->code == ABS_Y) /* Legacy calibrated absolute Y. */
        pointer->y = ((double)event->value - pointer->ay.minimum) * (DEMO_HEIGHT - 1) / /* Map once to world. */
            ((double)pointer->ay.maximum - pointer->ay.minimum); /* No display offset added to world. */
    if (event->type == EV_KEY && (event->code == BTN_LEFT || event->code == BTN_TOUCH) && /* One primary stream. */
        (event->value == FB_DEMO_KEY_RELEASED || event->value == FB_DEMO_KEY_PRESSED)) /* Ignore autorepeat and unrelated keyboard events. */
        return pointer_send(pointer, demo, event->value ? FVM_MOUSE_EVENT_DOWN : FVM_MOUSE_EVENT_UP); /* Ordered. */
    if (event->type == EV_SYN && event->code == SYN_REPORT) { /* Commit one completed native packet. */
        FVM_Affine identity = {1, 0, 0, 1, 0, 0}; /* Framebuffer rectangle and world have equal pixel units. */
        double delta = ((pointer->wheel_highres_available || pointer->wheel_has_highres) ? /* Select one representation for this report. */
            pointer->wheel_highres / FB_DEMO_WHEEL_HI_RES_PER_DETENT : pointer->wheel_legacy) * /* Detents. */
            DEMO_WHEEL_LINES_PER_DETENT; /* Explicit tutorial line policy, independent of OS preferences. */
        pointer->wheel_legacy = pointer->wheel_highres = 0; pointer->wheel_has_highres = 0; /* End packet. */
        if (!pointer_send(pointer, demo, FVM_MOUSE_EVENT_MOVE)) return 0; /* Clamp/publish completed position first. */
        if (delta != 0) return demo_wheel(demo, &identity, pointer->x, pointer->y, delta) == FVM_OK; /* Vertical lines. */
        return 1; /* No wheel event for horizontal-only, zero or ordinary motion reports. */
    } /* Legacy button notifications retain their documented native record order. */
    return 1; /* Other records have no defined mapping in this pointer tutorial. */
} /* ABS_MT_SLOT/TRACKING_ID protocols are not decoded here. */
/* Assemble default three-byte mousedev packets, preserving partial reads and sign bits. */
static int mouse_byte(PointerDevice *pointer, Demo *demo, unsigned char value) { /* No IntelliMouse negotiation. */
    int held; /* New primary button state after complete packet. */
    if (!pointer->used && !(value & FB_DEMO_MOUSE_SYNC)) return 1; /* Seek the next packet boundary. */
    pointer->packet[pointer->used++] = value; /* At most one bounded packet is assembled. */
    if (pointer->used < FB_DEMO_MOUSE_BYTES) return 1; /* Keep fragment until the next read. */
    pointer->used = 0; /* Next byte starts a new packet, even if this one fails validation. */
    if (pointer->packet[0] & FB_DEMO_MOUSE_OVERFLOW) { errno = EOVERFLOW; return 0; } /* No invented delta. */
    pointer->x += (int)pointer->packet[1] - /* PS/2 sign bit lives in the header, not signed char. */
        ((pointer->packet[0] & FB_DEMO_MOUSE_X_SIGN) ? FB_DEMO_BYTE_RANGE : 0); /* Signed X displacement. */
    pointer->y -= (int)pointer->packet[2] - /* PS/2 Y is up-positive; framebuffer Y is down-positive. */
        ((pointer->packet[0] & FB_DEMO_MOUSE_Y_SIGN) ? FB_DEMO_BYTE_RANGE : 0); /* Invert Y exactly once. */
    held = (pointer->packet[0] & FB_DEMO_MOUSE_LEFT) != 0; /* Copy physical primary-button state. */
    if (!pointer_send(pointer, demo, FVM_MOUSE_EVENT_MOVE)) return 0; /* Publish packet position first. */
    return held == demo->held || pointer_send(pointer, demo, held ? /* Only actual button changes dispatch. */
        FVM_MOUSE_EVENT_DOWN : FVM_MOUSE_EVENT_UP); /* Ignore middle/right buttons and wheel extensions. */
} /* Use the real mousedev node, not an evdev node with this decoder. */
/* Read bounded nonblocking input; EAGAIN and EINTR preserve partial state without failure. */
static int pointer_read(PointerDevice *pointer, Demo *demo) { /* Readiness is only a hint. */
    unsigned int i; /* Prevent an active device from starving the rendering loop. */
    for (i = 0; i < FB_DEMO_INPUT_BATCH; ++i) { /* At most 64 records/bytes per tick. */
        struct input_event event; /* evdev read must return a whole record. */
        unsigned char byte; /* mousedev stream can be split at any byte. */
        ssize_t count = read(pointer->fd, pointer->mouse ? (void *)&byte : (void *)&event, /* Explicit mode. */
            pointer->mouse ? sizeof(byte) : sizeof(event)); /* Never interpret one ABI as another. */
        if (count < 0) return errno == EAGAIN || errno == EINTR; /* Retry in next owner tick. */
        if (count != (ssize_t)(pointer->mouse ? sizeof(byte) : sizeof(event))) return 0; /* EOF/short evdev fatal. */
        if (pointer->mouse ? !mouse_byte(pointer, demo, byte) : !evdev_event(pointer, demo, &event)) return 0; /* Dispatch. */
    } /* Continuous input still yields to the next frame. */
    return 1; /* More pending input remains ready for next tick. */
} /* No background thread enters FlashVM. */
/* Open one explicitly selected pointer source and query optional ABS ranges. */
static int pointer_open(PointerDevice *pointer, const char *path, int mouse) { /* Caller owns cleanup. */
    pointer->fd = open(path, O_RDONLY | O_NONBLOCK); /* No exclusive grab or keyboard permission changes. */
    if (pointer->fd < 0) return 0; /* Missing/inaccessible node is an actual error. */
    pointer->mouse = mouse; pointer->x = DEMO_WIDTH / 2; pointer->y = DEMO_HEIGHT / 2; /* Initial center. */
    if (!mouse) { /* Never issue evdev ioctls against a PS/2 byte stream. */
        int version; /* EVIOCGVERSION rejects ordinary files and non-evdev device nodes. */
#ifdef REL_WHEEL_HI_RES /* Select one authoritative wheel protocol per device, not only per report. */
        unsigned char relative[FB_DEMO_REL_BITS_BYTES]; /* Zero unused capability bits before ioctl. */
        memset(relative, 0, sizeof(relative)); /* Older kernels may return a shorter bit span. */
        if (ioctl(pointer->fd, EVIOCGBIT(EV_REL, sizeof(relative)), relative) < 0) return 0; /* Query actual axes. */
        pointer->wheel_highres_available = (relative[REL_WHEEL_HI_RES / CHAR_BIT] & /* High-resolution axis bit. */
            (1U << (REL_WHEEL_HI_RES % CHAR_BIT))) != 0; /* Do not count its legacy compatibility packets. */
#endif /* Older header SDKs explicitly use legacy detents only. */
        if (ioctl(pointer->fd, EVIOCGVERSION, &version) < 0) return 0; /* Validate selected ABI. */
        pointer->absolute = ioctl(pointer->fd, EVIOCGABS(ABS_X), &pointer->ax) == 0 && /* Optional legacy X. */
            ioctl(pointer->fd, EVIOCGABS(ABS_Y), &pointer->ay) == 0 && /* Both calibrated axes required. */
            pointer->ax.maximum > pointer->ax.minimum && pointer->ay.maximum > pointer->ay.minimum; /* Nonzero ranges. */
    } /* Pure multitouch devices need a separate slot-aware adapter, not this legacy path. */
    return 1; /* Main reports unsupported setup rather than silently falling back to another source. */
} /* Native held-at-open state is not synthesized; begin with released contact. */
/* Parse a positive frame count without accepting overflow, suffixes or a negative value. */
static int frame_limit(const char *text, unsigned long *out) { /* Zero is reserved for interactive default. */
    char *end; /* strtoul reports the first unparsed byte. */
    unsigned long value; /* Native frame count uses unsigned long. */
    if (!*text || *text == '-') return 0; /* Reject empty/negative arguments. */
    errno = 0; value = strtoul(text, &end, 10); /* Exact decimal input with ERANGE checking. */
    if (errno || *end || !value) return 0; /* No partial or unbounded explicit limit. */
    *out = value; return 1; /* Caller owns the destination. */
} /* No shell/device discovery is performed by this example. */
/* All device paths are explicit; the default framebuffer alone requires no input device. */
int main(int argc, char **argv) { /* Real Linux device integration, no display server required. */
    LinuxHost host; Demo demo; /* Stack owners outlive all synchronous callbacks. */
    const char *fb_path = "/dev/fb0", *input_path = NULL, *video_path = NULL; /* Never open camera implicitly. */
    unsigned long limit = 0, frames = 0; /* Interactive until signal unless --frames is supplied. */
    int mouse = 0, i, result = 1; /* Nonzero status until every requested operation succeeds. */
    struct sigaction action; /* Controlled process stop with reverse-order cleanup. */
    memset(&host, 0, sizeof(host)); memset(&demo, 0, sizeof(demo)); /* Partial acquisitions are safe. */
    host.fb.fd = host.video.fd = host.pointer.fd = -1; /* Descriptor zero is a valid acquisition. */
    for (i = 1; i < argc; ++i) { /* Require one value for each supported option. */
        if (i + 1 >= argc) goto usage; /* No missing-value dereference. */
        if (!strcmp(argv[i], "--fb")) fb_path = argv[++i]; /* Existing framebuffer mode only. */
        else if (!strcmp(argv[i], "--video")) video_path = argv[++i]; /* Opt-in camera capture. */
        else if (!strcmp(argv[i], "--frames")) { if (!frame_limit(argv[++i], &limit)) goto usage; } /* Finite run. */
        else if ((!strcmp(argv[i], "--evdev") || !strcmp(argv[i], "--mouse")) && !input_path) { /* One source. */
            mouse = !strcmp(argv[i], "--mouse"); input_path = argv[++i]; /* ABI is selected explicitly. */
        } else goto usage; /* Unknown or duplicate input source is an error. */
    } /* No arguments still means framebuffer-only interactive rendering. */
    if (!fb_open(&host.fb, fb_path)) { fprintf(stderr, "unsupported/inaccessible framebuffer: %s\n", fb_path); goto cleanup; } /* Real node. */
    if (input_path && !pointer_open(&host.pointer, input_path, mouse)) { perror("pointer open"); goto cleanup; } /* No fallback. */
    if (video_path && !video_open(&host.video, video_path)) { fprintf(stderr, "V4L2 requires progressive single-plane MMAP YUYV: %s\n", video_path); goto cleanup; } /* Format. */
    if (demo_open(&demo) != FVM_OK) goto cleanup; /* Acquire VM only after native initialization succeeds. */
    memset(&action, 0, sizeof(action)); action.sa_handler = request_stop; sigemptyset(&action.sa_mask); /* Signal-safe stop. */
    if (sigaction(SIGINT, &action, NULL) || sigaction(SIGTERM, &action, NULL)) goto cleanup; /* No VM in handlers. */
    result = 0; /* Later device/presentation failures explicitly restore nonzero. */
    while (!stopping && (!limit || frames < limit)) { /* One owner thread processes all protocols. */
        struct pollfd ready[FB_DEMO_POLL_SOURCES]; /* Negative fds are ignored by poll. */
        int status; /* Native readiness result. */
        ready[0].fd = host.pointer.fd; ready[0].events = POLLIN; ready[0].revents = 0; /* Input readiness. */
        ready[1].fd = host.video.fd; ready[1].events = POLLIN; ready[1].revents = 0; /* Optional camera. */
        status = poll(ready, FB_DEMO_POLL_SOURCES, FB_DEMO_POLL_MS); /* Bounded wait, no camera blocks VM rendering. */
        if (status < 0 && errno == EINTR) continue; /* Stop flag is rechecked at loop boundary. */
        if (status < 0 || (ready[0].revents & (POLLERR | POLLHUP | POLLNVAL)) || /* Device lost. */
            (ready[1].revents & (POLLERR | POLLHUP | POLLNVAL))) { result = 1; break; } /* Stop instead of spinning. */
        if ((ready[0].revents & POLLIN) && !pointer_read(&host.pointer, &demo)) { result = 1; break; } /* Bounded input. */
        if ((ready[1].revents & POLLIN) && !video_step(&host.video)) { result = 1; break; } /* Return capture slot. */
        if (demo_frame(&demo, linux_present, &host) != FVM_OK) { result = 1; break; } /* Consume real mapped frame. */
        ++frames; /* Count completed host writes, not physical refreshes. */
    } /* Stop input/capture work before releasing the VM. */
    demo_cancel(&demo); /* Finish a held contact while context is still live. */
    if (demo.status != FVM_OK) result = 1; /* Cancellation errors remain visible. */
    printf("linux_fb frames=%lu camera_frames=%u wheels=%u last_delta=%.3f status=%d\n", frames, host.video.frames, demo.wheel_events, demo.last_wheel_delta, (int)demo.status); /* Evidence. */
    goto cleanup; /* Both normal and failure paths release all owners. */
usage: /* Invalid arguments open no devices. */
    fprintf(stderr, "usage: %s [--fb /dev/fb0] [--evdev /dev/input/eventN | --mouse /dev/input/mouse0] [--video /dev/videoN] [--frames N]\n", argv[0]); /* Exact ABI. */
cleanup: /* Reverse native/VM ownership, safe on every partial setup path. */
    if (result) fprintf(stderr, "linux_fb failed: VM=%d errno=%d\n", (int)demo.status, errno); /* Do not fake success. */
    demo_close(&demo); /* [GC:] No managed pointer or native callback remains active. */
    video_close(&host.video); /* Stop streaming before unmapping driver buffers. */
    if (host.pointer.fd >= 0) close(host.pointer.fd); /* Release the one optional input descriptor. */
    fb_close(&host.fb); /* Unmap display memory before closing its backing descriptor. */
    return result; /* Permissions/device/format failures are nonzero. */
} /* Do not run concurrently with another owner writing the same framebuffer. */

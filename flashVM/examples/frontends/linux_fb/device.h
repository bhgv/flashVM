/* Linux-only framebuffer, evdev/mousedev and single-plane V4L2 helpers for this tutorial. */
#ifndef FVM_LINUX_DEMO_DEVICE_H /* Keep native helpers private to one host executable. */
#define FVM_LINUX_DEMO_DEVICE_H /* No kernel structure crosses the FlashVM ABI. */
#include "../common/wheel.h" /* Portable VM/frame owner. */
#include <linux/fb.h> /* Native framebuffer layout queries. */
#include <linux/input.h> /* evdev records, relative axes and primary button. */
#include <time.h> /* Supply timespec before the V4L2 ABI header. */
#include <linux/videodev2.h> /* Single-plane mmap capture ABI. */
#include <sys/ioctl.h> /* Device-specific query/control calls. */
#include <sys/mman.h> /* Map framebuffer and driver-owned capture buffers. */
#include <unistd.h> /* Read and close native descriptors. */
#include <fcntl.h> /* Explicit descriptor modes, including nonblocking input. */
#include <errno.h> /* Distinguish retryable readiness from fatal errors. */
#include <limits.h> /* Validate mapping arithmetic without integer overflow. */
#define FB_DEMO_RGB565_BITS 16U /* Supported two-byte truecolor depth. */
#define FB_DEMO_RGB888_BITS 24U /* Supported three-byte truecolor depth. */
#define FB_DEMO_RGBA8888_BITS 32U /* Supported four-byte truecolor depth. */
#define FB_DEMO_CHANNEL_BITS 8U /* Maximum channel precision accepted by the pixel packer. */
#define FB_DEMO_YUYV_PAIR_PIXELS 2U /* Chroma-shared YUYV pairs require an even pixel width. */
#define FB_DEMO_POLL_SOURCES 2U /* One pointer and one optional camera descriptor. */
#define FB_DEMO_KEY_RELEASED 0 /* evdev primary button release value. */
#define FB_DEMO_KEY_PRESSED 1 /* evdev primary button press value; repeat is not a transition. */
#define FB_DEMO_REL_BITS_BYTES ((REL_MAX + CHAR_BIT) / CHAR_BIT) /* Byte span for relative-axis capability bits. */
#define FB_DEMO_WHEEL_HI_RES_PER_DETENT 120.0 /* evdev high-resolution counts for one wheel detent. */
#define FB_DEMO_MAX_BUFFERS 8U /* Bound native camera mappings regardless of driver count. */
#define FB_DEMO_REQUEST_BUFFERS 2U /* Minimum useful streaming queue for this example. */
#define FB_DEMO_MAX_VIDEO_WIDTH 1920U /* Bound negotiated capture dimensions and row arithmetic. */
#define FB_DEMO_MAX_VIDEO_HEIGHT 1080U /* This is not a general-purpose video decoder. */
#define FB_DEMO_YUYV_BYTES 2U /* Packed YUYV stores two bytes per pixel on average. */
#define FB_DEMO_MOUSE_BYTES 3U /* Standard non-wheel mousedev PS/2 packet length. */
#define FB_DEMO_MOUSE_SYNC 8U /* Mandatory synchronization bit in the first PS/2 byte. */
#define FB_DEMO_MOUSE_OVERFLOW 192U /* X/Y overflow flags invalidate packet deltas. */
#define FB_DEMO_MOUSE_X_SIGN 16U /* Sign extension bit for the X displacement. */
#define FB_DEMO_MOUSE_Y_SIGN 32U /* Sign extension bit for the Y displacement. */
#define FB_DEMO_MOUSE_LEFT 1U /* Primary button in the PS/2 packet header. */
#define FB_DEMO_BYTE_RANGE 256 /* Signed nine-bit mousedev delta adjustment. */
#define FB_DEMO_INPUT_BATCH 64U /* Bound input work per owner-loop tick. */
/* Framebuffer mapping owns no VM objects and never changes the display mode. */
typedef struct FbDevice { /* Initialize fd to -1 before opening. */
    int fd; /* Owned framebuffer descriptor. */
    unsigned char *map; /* Owned mapping, or NULL before mmap succeeds. */
    struct fb_fix_screeninfo fixed; /* Driver-reported stride, memory span and visual. */
    struct fb_var_screeninfo mode; /* Driver-reported dimensions, offsets and bitfields. */
} FbDevice; /* Writes affect only the fixed 320x200 visible tutorial rectangle. */
/* Validate channel fields before shifting; indexed/directcolor palettes are not supported. */
static int fb_field_valid(struct fb_bitfield field, unsigned int bits) { /* Scalar validation only. */
    return field.length > 0 && field.length <= FB_DEMO_CHANNEL_BITS && field.offset < bits && /* RGB channels must exist. */
        field.length <= bits - field.offset && !field.msb_right; /* Reject reversed/overflowing fields. */
} /* Length bounds also make packing shifts safe. */
/* Validate every address used by the fixed-size presenter before any device write. */
static int fb_layout_valid(const struct fb_fix_screeninfo *fixed, const struct fb_var_screeninfo *mode) { /* Testable. */
    unsigned long bytes, end_x, end_y, row_end; /* Use checked arithmetic within ABI widths. */
    unsigned long r, g, b, a; /* Channel masks must not overlap. */
    unsigned short endian = 1; /* Packed byte writes below explicitly support little-endian hosts. */
    if (*(unsigned char *)&endian != 1 || fixed->type != FB_TYPE_PACKED_PIXELS || /* No planar/BE path. */
        fixed->visual != FB_VISUAL_TRUECOLOR || mode->nonstd || mode->grayscale || mode->rotate || /* No palette/FourCC mode. */
        (mode->bits_per_pixel != FB_DEMO_RGB565_BITS && mode->bits_per_pixel != FB_DEMO_RGB888_BITS && /* Packed depths supported by this presenter. */
         mode->bits_per_pixel != FB_DEMO_RGBA8888_BITS) || /* Supported. */
        mode->xres < DEMO_WIDTH || mode->yres < DEMO_HEIGHT) return 0; /* No unsupported mode mutation. */
    if (!fb_field_valid(mode->red, mode->bits_per_pixel) || /* Validate each packed RGB field. */
        !fb_field_valid(mode->green, mode->bits_per_pixel) || /* Green may be RGB565's six bits. */
        !fb_field_valid(mode->blue, mode->bits_per_pixel)) return 0; /* Reject unsafe channel packing. */
    if (mode->transp.length && !fb_field_valid(mode->transp, mode->bits_per_pixel)) return 0; /* Optional alpha. */
    r = ((1UL << mode->red.length) - 1) << mode->red.offset; /* Safe after field validation. */
    g = ((1UL << mode->green.length) - 1) << mode->green.offset; /* Green packed mask. */
    b = ((1UL << mode->blue.length) - 1) << mode->blue.offset; /* Blue packed mask. */
    a = mode->transp.length ? ((1UL << mode->transp.length) - 1) << mode->transp.offset : 0; /* Optional. */
    if ((r & g) || (r & b) || (g & b) || (a & (r | g | b))) return 0; /* No aliased bitfields. */
    bytes = mode->bits_per_pixel / CHAR_BIT; /* Bytes per packed pixel. */
    end_x = (unsigned long)mode->xoffset + DEMO_WIDTH; /* Exclusive right edge in virtual pixels. */
    end_y = (unsigned long)mode->yoffset + DEMO_HEIGHT; /* Exclusive bottom edge. */
    if (end_x < mode->xoffset || end_y < mode->yoffset) return 0; /* Detect unsigned wrap before bounds checks. */
    if (end_x > mode->xres_virtual || end_y > mode->yres_virtual || end_x > ULONG_MAX / bytes) return 0; /* Bounds. */
    row_end = end_x * bytes; /* Exclusive byte position within the final row. */
    if (!fixed->line_length || row_end > fixed->line_length || /* Respect device padding. */
        end_y - 1 > (ULONG_MAX - row_end) / fixed->line_length) return 0; /* Last-row overflow guard. */
    return (end_y - 1) * fixed->line_length + row_end <= fixed->smem_len; /* Actual mapped device span. */
} /* No framebuffer access occurs during layout validation. */
/* Pack one byte channel into the validated native bitfield. */
static unsigned long fb_channel(unsigned int value, struct fb_bitfield field) { /* Opaque RGB input. */
    return field.length ? ((value * ((1UL << field.length) - 1) / DEMO_OPAQUE) << field.offset) : 0; /* Quantize. */
} /* Integer scaling also supports RGB565. */
/* Open and map the existing display mode, never FBIOPUT_VSCREENINFO. */
static int fb_open(FbDevice *fb, const char *path) { /* Caller provides zeroed partial owner. */
    fb->fd = open(path, O_RDWR); /* Explicit device path; permissions are caller managed. */
    if (fb->fd < 0 || ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fixed) < 0 || /* Query memory layout. */
        ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->mode) < 0 || /* Query native format and virtual offsets. */
        !fb_layout_valid(&fb->fixed, &fb->mode)) return 0; /* Reject before mapping/writing. */
    fb->map = (unsigned char *)mmap(NULL, fb->fixed.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0); /* Map. */
    if (fb->map == MAP_FAILED) { fb->map = NULL; return 0; } /* Preserve cleanup-safe sentinel. */
    return 1; /* All future framebuffer addresses have been validated. */
} /* Existing display contents are not saved or restored by this small tutorial. */
/* Release mapping before its backing descriptor. */
static void fb_close(FbDevice *fb) { /* Safe after a partially failed fb_open. */
    if (fb->map) munmap(fb->map, fb->fixed.smem_len); /* End visible device-memory borrowing. */
    if (fb->fd >= 0) close(fb->fd); /* Release descriptor after mapping teardown. */
} /* Caller no longer uses fb after cleanup. */
/* Each capture buffer is borrowed from the driver between DQBUF and QBUF. */
typedef struct VideoBuffer { /* Mapping state survives readiness callbacks. */
    void *data; /* Owned mmap or NULL. */
    size_t length; /* Exact driver mapping length for bounds and munmap. */
} VideoBuffer; /* No VM storage is handed to V4L2. */
/* Explicit single-plane YUYV capture with a grayscale preview copied into host-owned storage. */
typedef struct VideoDevice { /* Initialize fd to -1 and all mappings to zero. */
    int fd; /* Owned optional /dev/videoN descriptor. */
    int streaming; /* STREAMOFF is needed only after successful STREAMON. */
    unsigned int count; /* Number of acquired mmap buffers, bounded by MAX_BUFFERS. */
    struct v4l2_format format; /* Actual negotiated format, never assumed requested dimensions. */
    VideoBuffer buffers[FB_DEMO_MAX_BUFFERS]; /* Partial acquisitions are individually releasable. */
    unsigned char preview[DEMO_WIDTH * DEMO_HEIGHT]; /* Native copied Y luminance; no color conversion claim. */
    unsigned int frames; /* Successfully copied camera frames, independent of presentation count. */
} VideoDevice; /* Camera data is optional, not a FlashVM device API. */
/* Retry an interrupted ioctl without repeating successful state transitions. */
static int video_ioctl(int fd, unsigned long request, void *value) { /* Borrow mutable ioctl payload. */
    int result; /* Preserve the actual final syscall result and errno. */
    do { result = ioctl(fd, request, value); } while (result < 0 && errno == EINTR); /* Signals may interrupt. */
    return result; /* Other failures remain visible to caller. */
} /* No retry on EAGAIN or protocol errors. */
/* Negotiate and queue a bounded single-plane MMAP YUYV stream. */
static int video_open(VideoDevice *video, const char *path) { /* Partial ownership remains caller-cleanable. */
    struct v4l2_capability caps; /* Driver capabilities, including device_caps override. */
    struct v4l2_requestbuffers request; /* Request explicit MMAP buffers. */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* Single-plane capture only. */
    unsigned int flags, i; /* Capabilities and mapping loop counter. */
    video->fd = open(path, O_RDWR | O_NONBLOCK); /* Never block the VM owner on frame delivery. */
    memset(&caps, 0, sizeof(caps)); /* Kernel structures start completely initialized. */
    if (video->fd < 0 || video_ioctl(video->fd, VIDIOC_QUERYCAP, &caps) < 0) return 0; /* Actual device. */
    flags = caps.capabilities & V4L2_CAP_DEVICE_CAPS ? caps.device_caps : caps.capabilities; /* Per-node flags. */
    if (!(flags & V4L2_CAP_VIDEO_CAPTURE) || !(flags & V4L2_CAP_STREAMING)) return 0; /* No mplane/read fallback. */
    video->format.type = type; /* Select capture plane for S_FMT. */
    video->format.fmt.pix.width = DEMO_WIDTH; /* Request small preview, accept checked negotiated size. */
    video->format.fmt.pix.height = DEMO_HEIGHT; /* No assumption that hardware honors exact dimensions. */
    video->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; /* Preview reads packed luma bytes only. */
    video->format.fmt.pix.field = V4L2_FIELD_NONE; /* Require progressive frames for simple row copying. */
    if (video_ioctl(video->fd, VIDIOC_S_FMT, &video->format) < 0 || /* Obtain actual driver format. */
        video->format.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV || /* Compressed/RGB formats are unsupported. */
        video->format.fmt.pix.field != V4L2_FIELD_NONE || /* Reject interlaced layout ambiguity. */
        !video->format.fmt.pix.width || !video->format.fmt.pix.height || (video->format.fmt.pix.width % FB_DEMO_YUYV_PAIR_PIXELS) || /* No empty frames. */
        video->format.fmt.pix.width > FB_DEMO_MAX_VIDEO_WIDTH || /* Bound all later row arithmetic. */
        video->format.fmt.pix.height > FB_DEMO_MAX_VIDEO_HEIGHT || /* Bound vertical dimensions too. */
        video->format.fmt.pix.bytesperline < video->format.fmt.pix.width * FB_DEMO_YUYV_BYTES) return 0; /* Pitch. */
    memset(&request, 0, sizeof(request)); /* Required kernel reserved fields stay zero. */
    request.type = type; request.memory = V4L2_MEMORY_MMAP; /* Select capture MMAP buffers. */
    request.count = FB_DEMO_REQUEST_BUFFERS; /* Driver may return a different bounded count. */
    if (video_ioctl(video->fd, VIDIOC_REQBUFS, &request) < 0 || !request.count || /* Require real buffers. */
        request.count > FB_DEMO_MAX_BUFFERS) return 0; /* Descriptor close releases rejected allocation. */
    video->count = request.count; /* Cleanup handles every successfully mapped slot. */
    for (i = 0; i < video->count; ++i) { /* Map then queue each driver-owned buffer. */
        struct v4l2_buffer buffer; /* Plain C89 block-local declaration. */
        memset(&buffer, 0, sizeof(buffer)); /* Initialize reserved fields. */
        buffer.type = type; buffer.memory = V4L2_MEMORY_MMAP; buffer.index = i; /* Identify slot. */
        if (video_ioctl(video->fd, VIDIOC_QUERYBUF, &buffer) < 0 || !buffer.length) return 0; /* Bounds. */
        video->buffers[i].length = buffer.length; /* Exact mmap span. */
        video->buffers[i].data = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, /* Driver memory. */
            video->fd, buffer.m.offset); /* Offset is returned by QUERYBUF, never guessed. */
        if (video->buffers[i].data == MAP_FAILED) { video->buffers[i].data = NULL; return 0; } /* Partial-safe. */
        if (video_ioctl(video->fd, VIDIOC_QBUF, &buffer) < 0) return 0; /* Driver owns queued buffer. */
    } /* No frame is read while still queued. */
    if (video_ioctl(video->fd, VIDIOC_STREAMON, &type) < 0) return 0; /* Start only after all slots queued. */
    video->streaming = 1; /* Cleanup now pairs STREAMOFF. */
    return 1; /* Readiness is polled by the same VM owner loop. */
} /* Neither this helper nor V4L2 calls the VM. */
/* Validate and copy luma from a driver frame into an independent bounded preview. */
static int video_copy(VideoDevice *video, const unsigned char *data, size_t length) { /* Testable no-device step. */
    unsigned int x, y, width = video->format.fmt.pix.width, height = video->format.fmt.pix.height; /* Actual size. */
    size_t stride = video->format.fmt.pix.bytesperline, row = width * FB_DEMO_YUYV_BYTES; /* Checked by open. */
    if (!width || !height || width > FB_DEMO_MAX_VIDEO_WIDTH || height > FB_DEMO_MAX_VIDEO_HEIGHT || /* Bounds. */
        stride < row || row > length || height - 1 > (length - row) / stride) return 0; /* Span guard. */
    for (y = 0; y < DEMO_HEIGHT; ++y) /* Nearest-neighbor preview rows. */
        for (x = 0; x < DEMO_WIDTH; ++x) /* Copy only Y, explicitly no colorspace conversion. */
            video->preview[y * DEMO_WIDTH + x] = data[(y * height / DEMO_HEIGHT) * stride + /* Native pitch. */
                (x * width / DEMO_WIDTH) * FB_DEMO_YUYV_BYTES]; /* Y occurs at every even byte in YUYV. */
    ++video->frames; /* Count only successful complete copies. */
    return 1; /* Driver bytes are no longer needed after return. */
} /* Preview luminance retains the device's raw Y range. */
/* Dequeue at most one frame per tick, always returning an acquired slot to the driver. */
static int video_step(VideoDevice *video) { /* Called only after POLLIN or as a nonblocking probe. */
    struct v4l2_buffer buffer; /* Returned index and bytesused belong to this dequeue. */
    int ok; /* Preserve conversion status through QBUF. */
    memset(&buffer, 0, sizeof(buffer)); /* Kernel reserved fields stay zero. */
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buffer.memory = V4L2_MEMORY_MMAP; /* Select queue. */
    if (video_ioctl(video->fd, VIDIOC_DQBUF, &buffer) < 0) return errno == EAGAIN; /* Not-ready is not failure. */
    if (buffer.index >= video->count) return 0; /* Invalid driver index cannot be safely requeued. */
    ok = !(buffer.flags & V4L2_BUF_FLAG_ERROR) && buffer.bytesused <= video->buffers[buffer.index].length && /* Span. */
        video_copy(video, (const unsigned char *)video->buffers[buffer.index].data, buffer.bytesused); /* Own copy. */
    if (video_ioctl(video->fd, VIDIOC_QBUF, &buffer) < 0) return 0; /* Return even a rejected frame slot. */
    return ok; /* Protocol/copy failures stop the tutorial instead of showing success. */
} /* Host never retains a dequeued driver pointer across loop iterations. */
/* Stop DMA before unmapping driver-owned memory and closing its descriptor. */
static void video_close(VideoDevice *video) { /* Safe for each partial acquisition path. */
    unsigned int i; /* Traverse only the bounded requested slots. */
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* Pair STREAMON queue type. */
    if (video->streaming) video_ioctl(video->fd, VIDIOC_STREAMOFF, &type); /* Stop before unmap. */
    for (i = 0; i < video->count; ++i) /* Successfully mapped slots are independently owned. */
        if (video->buffers[i].data) munmap(video->buffers[i].data, video->buffers[i].length); /* Release mapping. */
    if (video->fd >= 0) close(video->fd); /* Releases remaining driver queue allocation too. */
} /* No camera work survives this owner. */
#endif /* Linux-only device helpers end here. */

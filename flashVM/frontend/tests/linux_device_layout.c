/* Device-free tests of the exact Linux tutorial decoders and layout checks. */
#define main linux_tutorial_entry
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/linux_fb/main.c"

#undef main


int main(void)
{
    struct fb_fix_screeninfo f;
    struct fb_var_screeninfo v;
    VideoDevice video;
    PointerDevice pointer;
    Demo demo;
    unsigned char pixels[16];
    struct input_event e;
    LinuxHost host;
    unsigned char *output;
    unsigned int x, before_loss;
    FVM_Affine singular = {0, 0, 0, 0, 0, 0};
    memset(&f, 0, sizeof(f));
    memset(&v, 0, sizeof(v));
    f.type = FB_TYPE_PACKED_PIXELS;
    f.visual = FB_VISUAL_TRUECOLOR;
    f.line_length = DEMO_WIDTH * 4 + 32;
    f.smem_len = f.line_length * (DEMO_HEIGHT + 1) + DEMO_WIDTH * 4;
    v.xres = v.xres_virtual = DEMO_WIDTH;
    v.yres = DEMO_HEIGHT;
    v.yres_virtual = DEMO_HEIGHT + 2;
    v.yoffset = 2;
    v.bits_per_pixel = 32;
    v.red.length = v.green.length = v.blue.length = v.transp.length = 8;
    v.red.offset = 16;
    v.green.offset = 8;
    v.transp.offset = 24;
    if (!(fb_layout_valid(&f, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fb_channel(255, v.red) == 0xff0000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&host, 0, sizeof(host));
    host.fb.fixed = f;
    host.fb.mode = v;
    output = (unsigned char *)malloc(f.smem_len);
    if (!(output))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(output, 0x5a, f.smem_len);
    host.fb.map = output;
    if (!(demo_open(&demo) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (x = 0; x < demo.byte_count; x += 4)
    {
        demo.frame.pixels[x] = 1;
        demo.frame.pixels[x + 1] = 2;
        demo.frame.pixels[x + 2] = 3;
        demo.frame.pixels[x + 3] = 255;
    }
    if (!(linux_present(&demo.frame, &host) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[0] == 0x5a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[2 * f.line_length] == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[2 * f.line_length + 1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[2 * f.line_length + 2] == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[2 * f.line_length + 3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[2 * f.line_length + DEMO_WIDTH * 4] == 0x5a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output[f.smem_len - 1] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    free(output);
    demo_close(&demo);
    --f.smem_len;
    if (!(!fb_layout_valid(&f, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++f.smem_len;
    v.red.offset = 8;
    if (!(!fb_layout_valid(&f, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.red.offset = 16;
    v.yoffset = UINT_MAX;
    if (!(!fb_layout_valid(&f, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.yoffset = 2;
    f.visual = FB_VISUAL_DIRECTCOLOR;
    if (!(!fb_layout_valid(&f, &v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&video, 0, sizeof(video));
    memset(pixels, 0, sizeof(pixels));
    video.format.fmt.pix.width = 2;
    video.format.fmt.pix.height = 2;
    video.format.fmt.pix.bytesperline = 8;
    pixels[0] = 20;
    pixels[2] = 40;
    pixels[8] = 60;
    pixels[10] = 80;
    if (!(!video_copy(&video, pixels, 11)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.frames == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video_copy(&video, pixels, 12)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.preview[0] == 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.preview[DEMO_WIDTH - 1] == 40))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.preview[(DEMO_HEIGHT - 1) * DEMO_WIDTH] == 60))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(video.preview[DEMO_WIDTH * DEMO_HEIGHT - 1] == 80))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_open(&demo) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&pointer, 0, sizeof(pointer));
    pointer.x = 100;
    pointer.y = 100;
    if (!(mouse_byte(&pointer, &demo, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.used == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(mouse_byte(&pointer, &demo, FB_DEMO_MOUSE_SYNC | FB_DEMO_MOUSE_LEFT | FB_DEMO_MOUSE_X_SIGN)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(mouse_byte(&pointer, &demo, 255)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.used == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(mouse_byte(&pointer, &demo, 2)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.used == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.x == 99 && pointer.y == 98 && demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = EV_REL;
    e.code = REL_X;
    e.value = 5;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.x == 104))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_REL;
    e.code = REL_WHEEL;
    e.value = 1;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef REL_WHEEL_HI_RES
    e.code = REL_WHEEL_HI_RES;
    e.value = 60;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    if (!(!demo.wheel_events))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_SYN;
    e.code = SYN_REPORT;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef REL_WHEEL_HI_RES
    if (!(demo.wheel_events == 1 && demo.last_wheel_delta == 1.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(demo.wheel_events == 1 && demo.last_wheel_delta == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    e.type = EV_REL;
    e.code = REL_WHEEL;
    e.value = -1;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_SYN;
    e.code = SYN_REPORT;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo.wheel_events == 2 && demo.last_wheel_delta == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef REL_WHEEL_HI_RES
    pointer.wheel_highres_available = 1;
    e.type = EV_REL;
    e.code = REL_WHEEL;
    e.value = 1;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_SYN;
    e.code = SYN_REPORT;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo.wheel_events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_REL;
    e.code = REL_WHEEL_HI_RES;
    e.value = -30;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_SYN;
    e.code = SYN_REPORT;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo.wheel_events == 3 && demo.last_wheel_delta == -0.75))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    before_loss = demo.wheel_events;
    e.type = EV_REL;
    e.code = REL_WHEEL;
    e.value = 1;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = EV_SYN;
    e.code = SYN_DROPPED;
    if (!(evdev_event(&pointer, &demo, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.dropping))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!demo.held && demo.wheel_events == before_loss))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pointer.wheel_legacy == 0 && pointer.wheel_highres == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before_loss = demo.wheel_events;
    if (!(demo_wheel(&demo, &singular, 0, 0, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo.wheel_events == before_loss))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    singular.xx = singular.yy = 1;
    if (!(demo_wheel(&demo, &singular, 0, 0, 1) == FVM_INVALID_ARGUMENT && demo.wheel_events == before_loss))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    demo_close(&demo);
    return 0;
}

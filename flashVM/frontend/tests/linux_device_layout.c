/* Device-free tests of the exact Linux tutorial decoders and layout checks. */
#define main linux_tutorial_entry
#include "../../examples/frontends/linux_fb/main.c"
#undef main
#include <assert.h>
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
    FVM_Affine singular={0,0,0,0,0,0};
    memset(&f,0,sizeof(f)); memset(&v,0,sizeof(v));
    f.type=FB_TYPE_PACKED_PIXELS; f.visual=FB_VISUAL_TRUECOLOR;
    f.line_length=DEMO_WIDTH*4+32; f.smem_len=f.line_length*(DEMO_HEIGHT+1)+DEMO_WIDTH*4;
    v.xres=v.xres_virtual=DEMO_WIDTH; v.yres=DEMO_HEIGHT; v.yres_virtual=DEMO_HEIGHT+2;
    v.yoffset=2; v.bits_per_pixel=32;
    v.red.length=v.green.length=v.blue.length=v.transp.length=8;
    v.red.offset=16; v.green.offset=8; v.transp.offset=24;
    assert(fb_layout_valid(&f,&v));
    assert(fb_channel(255,v.red)==0xff0000UL);
    memset(&host,0,sizeof(host)); host.fb.fixed=f; host.fb.mode=v;
    output=(unsigned char *)malloc(f.smem_len); assert(output);
    memset(output,0x5a,f.smem_len); host.fb.map=output;
    assert(demo_open(&demo)==FVM_OK);
    for(x=0;x<demo.byte_count;x+=4) {
        demo.frame.pixels[x]=1; demo.frame.pixels[x+1]=2;
        demo.frame.pixels[x+2]=3; demo.frame.pixels[x+3]=255;
    }
    assert(linux_present(&demo.frame,&host)==FVM_OK);
    assert(output[0]==0x5a); assert(output[2*f.line_length]==3);
    assert(output[2*f.line_length+1]==2); assert(output[2*f.line_length+2]==1);
    assert(output[2*f.line_length+3]==255);
    assert(output[2*f.line_length+DEMO_WIDTH*4]==0x5a);
    assert(output[f.smem_len-1]==255);
    free(output); demo_close(&demo);
    --f.smem_len; assert(!fb_layout_valid(&f,&v)); ++f.smem_len;
    v.red.offset=8; assert(!fb_layout_valid(&f,&v)); v.red.offset=16;
    v.yoffset=UINT_MAX; assert(!fb_layout_valid(&f,&v)); v.yoffset=2;
    f.visual=FB_VISUAL_DIRECTCOLOR; assert(!fb_layout_valid(&f,&v));
    memset(&video,0,sizeof(video)); memset(pixels,0,sizeof(pixels));
    video.format.fmt.pix.width=2; video.format.fmt.pix.height=2; video.format.fmt.pix.bytesperline=8;
    pixels[0]=20; pixels[2]=40; pixels[8]=60; pixels[10]=80;
    assert(!video_copy(&video,pixels,11)); assert(video.frames==0);
    assert(video_copy(&video,pixels,12)); assert(video.frames==1);
    assert(video.preview[0]==20); assert(video.preview[DEMO_WIDTH-1]==40);
    assert(video.preview[(DEMO_HEIGHT-1)*DEMO_WIDTH]==60);
    assert(video.preview[DEMO_WIDTH*DEMO_HEIGHT-1]==80);
    assert(demo_open(&demo)==FVM_OK); memset(&pointer,0,sizeof(pointer));
    pointer.x=100; pointer.y=100;
    assert(mouse_byte(&pointer,&demo,0)); assert(pointer.used==0);
    assert(mouse_byte(&pointer,&demo,FB_DEMO_MOUSE_SYNC|FB_DEMO_MOUSE_LEFT|FB_DEMO_MOUSE_X_SIGN));
    assert(mouse_byte(&pointer,&demo,255)); assert(pointer.used==2);
    assert(mouse_byte(&pointer,&demo,2)); assert(pointer.used==0);
    assert(pointer.x==99 && pointer.y==98 && demo.held);
    memset(&e,0,sizeof(e)); e.type=EV_REL; e.code=REL_X; e.value=5;
    assert(evdev_event(&pointer,&demo,&e)); assert(pointer.x==104);
    e.type=EV_REL;e.code=REL_WHEEL;e.value=1;assert(evdev_event(&pointer,&demo,&e));
#ifdef REL_WHEEL_HI_RES
    e.code=REL_WHEEL_HI_RES;e.value=60;assert(evdev_event(&pointer,&demo,&e));
#endif
    assert(!demo.wheel_events);
    e.type=EV_SYN;e.code=SYN_REPORT;assert(evdev_event(&pointer,&demo,&e));
#ifdef REL_WHEEL_HI_RES
    assert(demo.wheel_events==1 && demo.last_wheel_delta==1.5);
#else
    assert(demo.wheel_events==1 && demo.last_wheel_delta==3);
#endif
    e.type=EV_REL;e.code=REL_WHEEL;e.value=-1;assert(evdev_event(&pointer,&demo,&e));
    e.type=EV_SYN;e.code=SYN_REPORT;assert(evdev_event(&pointer,&demo,&e));
    assert(demo.wheel_events==2 && demo.last_wheel_delta==-3);
#ifdef REL_WHEEL_HI_RES
    pointer.wheel_highres_available=1;
    e.type=EV_REL;e.code=REL_WHEEL;e.value=1;assert(evdev_event(&pointer,&demo,&e));
    e.type=EV_SYN;e.code=SYN_REPORT;assert(evdev_event(&pointer,&demo,&e));
    assert(demo.wheel_events==2);
    e.type=EV_REL;e.code=REL_WHEEL_HI_RES;e.value=-30;assert(evdev_event(&pointer,&demo,&e));
    e.type=EV_SYN;e.code=SYN_REPORT;assert(evdev_event(&pointer,&demo,&e));
    assert(demo.wheel_events==3 && demo.last_wheel_delta==-0.75);
#endif
    before_loss=demo.wheel_events;
    e.type=EV_REL;e.code=REL_WHEEL;e.value=1;assert(evdev_event(&pointer,&demo,&e));
    e.type=EV_SYN; e.code=SYN_DROPPED;
    assert(!evdev_event(&pointer,&demo,&e)); assert(!demo.held && demo.wheel_events==before_loss);
    before_loss=demo.wheel_events;
    assert(demo_wheel(&demo,&singular,0,0,1)==FVM_INVALID_ARGUMENT);
    assert(demo.wheel_events==before_loss);
    singular.xx=singular.yy=1;
    assert(demo_wheel(&demo,&singular,0,0,1)==FVM_INVALID_ARGUMENT && demo.wheel_events==before_loss);
    demo_close(&demo);
    return 0;
}

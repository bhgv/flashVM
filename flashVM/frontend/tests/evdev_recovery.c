/* Kernel-free exercise of the real decoder's loss boundary and ioctl snapshots. */
#define main linux_tutorial_entry
#include "../../examples/frontends/linux_fb/main.c"
#undef main
#include <assert.h>
static int queries, failure, held;
static struct input_absinfo sx, sy;
static int snapshot(int fd, unsigned long request, void *out)
{
    (void)fd; ++queries;
    if (failure && queries == failure) { errno = ENODEV; return -1; }
    if (request == EVIOCGKEY(FB_DEMO_KEY_BITS_BYTES)) {
        unsigned char *keys = (unsigned char *)out;
        memset(keys,0,FB_DEMO_KEY_BITS_BYTES);
        if (held) keys[BTN_TOUCH/CHAR_BIT] |= (unsigned char)(1U << (BTN_TOUCH%CHAR_BIT));
    } else if (request == EVIOCGABS(ABS_X)) *(struct input_absinfo *)out = sx;
    else if (request == EVIOCGABS(ABS_Y)) *(struct input_absinfo *)out = sy;
    else assert(0);
    return 0;
}
static int event(PointerDevice *p, Demo *d, unsigned short type, unsigned short code, int value)
{
    struct input_event e;
    memset(&e,0,sizeof(e)); e.type=type;e.code=code;e.value=value;
    return evdev_event(p,d,&e);
}
static void position(Demo *d, double x, double y)
{
    double a,b;
    assert(fvm_input_get_pointer(d->context,&a,&b)==FVM_OK);
    assert(a==x && b==y);
}
int main(void)
{
    Demo d; PointerDevice p; int attempt;
    assert(demo_open(&d)==FVM_OK); memset(&p,0,sizeof(p));p.query=snapshot;
    p.x=100;p.y=80;
    assert(pointer_send(&p,&d,FVM_MOUSE_EVENT_DOWN));assert(d.held);
    assert(event(&p,&d,EV_REL,REL_X,17));
    assert(event(&p,&d,EV_REL,REL_WHEEL,2));
    assert(event(&p,&d,EV_SYN,SYN_DROPPED,0));
    assert(!d.held && p.dropping && p.x==100 && p.y==80);
    position(&d,100,80);assert(!queries && !d.wheel_events);
    assert(event(&p,&d,EV_REL,REL_X,1000));
    assert(event(&p,&d,EV_KEY,BTN_LEFT,1));
    assert(event(&p,&d,EV_REL,REL_WHEEL,50));
    assert(event(&p,&d,EV_SYN,SYN_MT_REPORT,0));
    assert(event(&p,&d,EV_SYN,SYN_DROPPED,0));
    assert(!queries && p.x==100 && !d.held);
    assert(event(&p,&d,EV_SYN,SYN_REPORT,0));
    assert(queries==1 && !p.dropping && !d.held && !d.wheel_events);
    position(&d,100,80);
    assert(event(&p,&d,EV_REL,REL_X,4));
    assert(event(&p,&d,EV_SYN,SYN_REPORT,0));position(&d,104,80);
    held=1; p.absolute=1;sx.minimum=0;sx.maximum=319;sx.value=45;
    sy.minimum=0;sy.maximum=199;sy.value=90;
    assert(event(&p,&d,EV_SYN,SYN_DROPPED,0));
    assert(event(&p,&d,EV_ABS,ABS_X,999)); /* discarded */
    assert(event(&p,&d,EV_SYN,SYN_REPORT,0));
    assert(queries==4 && d.held && !p.dropping);position(&d,45,90);
    /* Query failures cannot publish a partially acquired absolute position. */
    for(attempt=1;attempt<=3;++attempt) {
        assert(event(&p,&d,EV_SYN,SYN_DROPPED,0));
        queries=0;failure=attempt;sx.value=200;sy.value=150;
        assert(!event(&p,&d,EV_SYN,SYN_REPORT,0));
        assert(errno==ENODEV && p.dropping && !d.held);
        position(&d,45,90);
    }
    failure=0;queries=0;sx.maximum=sx.minimum;
    assert(!event(&p,&d,EV_SYN,SYN_REPORT,0));
    assert(errno==EINVAL && p.dropping);position(&d,45,90);
    sx.maximum=319;sx.value=999;sy.value=-100;
    assert(event(&p,&d,EV_SYN,SYN_REPORT,0));
    assert(d.held && !p.dropping);position(&d,319,0);
    demo_close(&d);return 0;
}

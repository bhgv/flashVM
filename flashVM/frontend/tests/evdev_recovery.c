/* Kernel-free exercise of the real decoder's loss boundary and ioctl snapshots. */
#define main linux_tutorial_entry
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/linux_fb/main.c"

#undef main

static int queries, failure, held;
static struct input_absinfo sx, sy;


static int snapshot(int fd, unsigned long request, void *out)
{
    (void)fd;
    ++queries;
    if (failure && queries == failure)
    {
        errno = ENODEV;
        return -1;
    }
    if (request == EVIOCGKEY(FB_DEMO_KEY_BITS_BYTES))
    {
        unsigned char *keys = (unsigned char *)out;
        memset(keys, 0, FB_DEMO_KEY_BITS_BYTES);
        if (held)
        {
            keys[BTN_TOUCH / CHAR_BIT] |= (unsigned char)(1U << (BTN_TOUCH % CHAR_BIT));
        }
    }
    else if (request == EVIOCGABS(ABS_X))
    {
        *(struct input_absinfo *)out = sx;
    }
    else if (request == EVIOCGABS(ABS_Y))
    {
        *(struct input_absinfo *)out = sy;
    }
    else
    {
        if (!(0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}


static int event(PointerDevice *p, Demo *d, unsigned short type, unsigned short code, int value)
{
    struct input_event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.code = code;
    e.value = value;
    return evdev_event(p, d, &e);
}


static void position(Demo *d, double x, double y)
{
    double a, b;
    if (!(fvm_input_get_pointer(d->context, &a, &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a == x && b == y))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    Demo d;
    PointerDevice p;
    int attempt;
    if (!(demo_open(&d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&p, 0, sizeof(p));
    p.query = snapshot;
    p.x = 100;
    p.y = 80;
    if (!(pointer_send(&p, &d, FVM_MOUSE_EVENT_DOWN)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_REL, REL_X, 17)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_REL, REL_WHEEL, 2)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_SYN, SYN_DROPPED, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!d.held && p.dropping && p.x == 100 && p.y == 80))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 100, 80);
    if (!(!queries && !d.wheel_events))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_REL, REL_X, 1000)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_KEY, BTN_LEFT, 1)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_REL, REL_WHEEL, 50)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_SYN, SYN_MT_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_SYN, SYN_DROPPED, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!queries && p.x == 100 && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_SYN, SYN_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(queries == 1 && !p.dropping && !d.held && !d.wheel_events))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 100, 80);
    if (!(event(&p, &d, EV_REL, REL_X, 4)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_SYN, SYN_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 104, 80);
    held = 1;
    p.absolute = 1;
    sx.minimum = 0;
    sx.maximum = 319;
    sx.value = 45;
    sy.minimum = 0;
    sy.maximum = 199;
    sy.value = 90;
    if (!(event(&p, &d, EV_SYN, SYN_DROPPED, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_ABS, ABS_X, 999)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* discarded */
    if (!(event(&p, &d, EV_SYN, SYN_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(queries == 4 && d.held && !p.dropping))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 45, 90);
    /* Query failures cannot publish a partially acquired absolute position. */
    for (attempt = 1; attempt <= 3; ++attempt)
    {
        if (!(event(&p, &d, EV_SYN, SYN_DROPPED, 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        queries = 0;
        failure = attempt;
        sx.value = 200;
        sy.value = 150;
        if (!(!event(&p, &d, EV_SYN, SYN_REPORT, 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(errno == ENODEV && p.dropping && !d.held))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(&d, 45, 90);
    }
    failure = 0;
    queries = 0;
    sx.maximum = sx.minimum;
    if (!(!event(&p, &d, EV_SYN, SYN_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(errno == EINVAL && p.dropping))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 45, 90);
    sx.maximum = 319;
    sx.value = 999;
    sy.value = -100;
    if (!(event(&p, &d, EV_SYN, SYN_REPORT, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(d.held && !p.dropping))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 319, 0);
    if (!(p.touch_down && !p.left_down))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event(&p, &d, EV_KEY, BTN_LEFT, 0) && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* untouched source cannot release recovered touch */
    if (!(event(&p, &d, EV_KEY, BTN_TOUCH, 0) && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    demo_close(&d);
    return 0;
}

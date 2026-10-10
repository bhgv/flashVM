#include "../input_port.h"
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
/* Linux evdev: set FVM_INPUT_DEVICE to a readable /dev/input/eventN.
 * Relative pointer motion is not converted to absolute screen coordinates. */
int fvm_port_mouse_poll(FVM_PortMouseState *state) {
    const char *path;
    int fd;
    unsigned char keys[(KEY_MAX + 8) / 8];
    if (!state) return 0;
    path = getenv("FVM_INPUT_DEVICE");
    if (!path || !*path) return 0;
    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return 0;
    memset(keys, 0, sizeof(keys));
    if (ioctl(fd, EVIOCGKEY(sizeof(keys)), keys) < 0) { close(fd); return 0; }
    close(fd);
#define FVM_KEY_DOWN(k) ((keys[(k)/8] >> ((k)%8)) & 1U)
    state->x = 0; state->y = 0;
    state->buttons = (FVM_KEY_DOWN(BTN_LEFT) ? 1U : 0U)
        | (FVM_KEY_DOWN(BTN_RIGHT) ? 2U : 0U)
        | (FVM_KEY_DOWN(BTN_MIDDLE) ? 4U : 0U);
#undef FVM_KEY_DOWN
    return 1;
}

int fvm_port_android_touch_poll(void *queue, unsigned int *fingers,
                                float *x, float *y, int *action) {
    (void)queue; (void)fingers; (void)x; (void)y; (void)action;
    return -1;
}

#include <stdio.h>
#include <stdlib.h>
#include "flashvm/multimedia.h"
#include <string.h>

#if defined(__linux__) && !defined(__ANDROID__)
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>

#endif


int main()
{
    FVM_DeviceEvent event;
    if (!(fvm_input_linux_evdev_read(-1, &event) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if defined(__linux__) && !defined(__ANDROID__)
    int fds[2];
    if (!(pipe(fds) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_linux_evdev_read(fds[0], &event) == FVM_NOT_FOUND && event.kind == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    struct input_event native;
    memset(&native, 0, sizeof(native));
    native.type = EV_KEY;
    native.code = KEY_A;
    native.value = 2;
    native.time.tv_sec = 42;
    if (!(write(fds[1], &native, sizeof(native)) == (ssize_t)sizeof(native)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_linux_evdev_read(fds[0], &event) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event.kind == FVM_DEVICE_KEY && event.native_code == KEY_A && event.x == 2 && event.seconds == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native.type = EV_ABS;
    native.code = ABS_MT_POSITION_X;
    native.value = 123;
    if (!(write(fds[1], &native, sizeof(native)) == (ssize_t)sizeof(native)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_linux_evdev_read(fds[0], &event) == FVM_OK && event.kind == FVM_DEVICE_TOUCH && event.x == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native.type = EV_REL;
    native.code = REL_X;
    native.value = -7;
    if (!(write(fds[1], &native, sizeof(native)) == (ssize_t)sizeof(native)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_linux_evdev_read(fds[0], &event) == FVM_OK && event.kind == FVM_DEVICE_MOUSE && event.x == -7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native.type = EV_SYN;
    if (!(write(fds[1], &native, sizeof(native)) == (ssize_t)sizeof(native)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_linux_evdev_read(fds[0], &event) == FVM_NOT_FOUND && event.kind == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(close(fds[0]) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(close(fds[1]) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(fvm_input_linux_evdev_read(0, &event) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}

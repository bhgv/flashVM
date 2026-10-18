#include "flashvm/multimedia.h"
#include <assert.h>
#include <string.h>
#if defined(__linux__) && !defined(__ANDROID__)
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#endif
int main()
{
    FVM_DeviceEvent event;
    assert(fvm_input_linux_evdev_read(-1,&event)==FVM_INVALID_ARGUMENT);
#if defined(__linux__) && !defined(__ANDROID__)
    int fds[2]; assert(pipe(fds)==0);
    assert(fcntl(fds[0],F_SETFL,O_NONBLOCK)==0);
    assert(fvm_input_linux_evdev_read(fds[0],&event)==FVM_NOT_FOUND && event.kind==0);
    struct input_event native; memset(&native,0,sizeof(native));
    native.type=EV_KEY; native.code=KEY_A; native.value=2; native.time.tv_sec=42;
    assert(write(fds[1],&native,sizeof(native))==(ssize_t)sizeof(native));
    assert(fvm_input_linux_evdev_read(fds[0],&event)==FVM_OK);
    assert(event.kind==FVM_DEVICE_KEY && event.native_code==KEY_A && event.x==2 && event.seconds==42);
    native.type=EV_ABS; native.code=ABS_MT_POSITION_X; native.value=123;
    assert(write(fds[1],&native,sizeof(native))==(ssize_t)sizeof(native));
    assert(fvm_input_linux_evdev_read(fds[0],&event)==FVM_OK && event.kind==FVM_DEVICE_TOUCH && event.x==123);
    native.type=EV_REL; native.code=REL_X; native.value=-7;
    assert(write(fds[1],&native,sizeof(native))==(ssize_t)sizeof(native));
    assert(fvm_input_linux_evdev_read(fds[0],&event)==FVM_OK && event.kind==FVM_DEVICE_MOUSE && event.x==-7);
    native.type=EV_SYN;
    assert(write(fds[1],&native,sizeof(native))==(ssize_t)sizeof(native));
    assert(fvm_input_linux_evdev_read(fds[0],&event)==FVM_NOT_FOUND && event.kind==0);
    assert(close(fds[0])==0); assert(close(fds[1])==0);
#else
    assert(fvm_input_linux_evdev_read(0,&event)==FVM_UNSUPPORTED);
#endif
    return 0;
}

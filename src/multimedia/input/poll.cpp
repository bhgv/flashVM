/* OS: host-selected. Poll a native queue or a future sensor adapter. */
#include "flashvm/multimedia.h" /* Import the device event ABI. */
#include <string.h> /* Clear unsuccessful event views. */
/* Poll one event without changing script input state or invoking ActionScript. */
FVM_Result fvm_device_poll(const FVM_DeviceBridge *bridge, /* Borrow the bridge for this call only. */
                           FVM_DeviceEvent *out) /* Borrow the bridge for this call only. */
{ /* Begin explicit device polling. */
    if (!out) /* Require a writable result slot. */
        return FVM_INVALID_ARGUMENT; /* Require a writable result slot. */
    memset(out, 0, sizeof(*out)); /* Never expose a stale event on failure. */
    if (!bridge || bridge->struct_size != sizeof(*bridge) || !bridge->poll) /* Validate the complete callback table. */
        return FVM_INVALID_ARGUMENT; /* Validate the complete callback table. */
    FVM_Result result = bridge->poll(bridge->userdata, out); /* Read one real host queue/device event. */
    if (result == FVM_OK && (out->kind < FVM_DEVICE_KEY || out->kind > FVM_DEVICE_ORIENTATION)) /* Reject unknown event categories. */
        result = FVM_ERROR; /* Reject unknown event categories. */
    if (result != FVM_OK) /* Clear partial callback output. */
        memset(out, 0, sizeof(*out)); /* Clear partial callback output. */
    return result; /* Preserve no-event and OS errors. */
} /* Finish the device bridge. */

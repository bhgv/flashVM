# Native OS ports (English)

`ports/input_port.h` and `ports/network_port.h` are C89 interfaces. Their implementations live in `ports/windows`, `ports/linux`, `ports/mac`, `ports/android` and are selected by CMake (`FVM_ENABLE_OS_PORTS`). The VM core has no OS dependencies.

## Input

`fvm_port_mouse_poll` samples real mouse button state and coordinates where available. Button bitmask: 1 left, 2 right, 4 middle. Windows uses `GetCursorPos` / `GetAsyncKeyState`. macOS uses CoreGraphics. Linux reads button state from the evdev device specified in `FVM_INPUT_DEVICE` (for example `/dev/input/eventN`); permissions are required, coordinates are **not** provided (zeroed). On Android there is no global cursor, so this call returns 0. Use `fvm_port_android_touch_poll` with a NativeActivity `AInputQueue *` to retrieve one motion event, its pointer count, first pointer position and Android action. The Activity owns the queue. The host is responsible for turning raw events into taps/swipes, using `fvm_input_dispatch` and the existing gesture-to-mouse mappings. No AVM1 mouse handler is automatically invoked yet.

## Network

`fvm_port_tcp_connect(host, port)` uses native OS TCP sockets and DNS (Winsock on Windows, BSD/POSIX sockets elsewhere). `send`/`recv` return byte counts, -1 on error; `recv` returns 0 on orderly close. Always call `fvm_port_socket_close`. **Calls are blocking**, no TLS, timeout, URL loading, Flash `XMLSocket`, `LoadVars`, or sandbox policy enforcement yet. Run on a dedicated host worker, never on the VM/render thread. Do not connect untrusted SWFs directly: implement origin permissions and limits before exposure to AVM1. Android requires `INTERNET` manifest permission. Windows uses Winsock startup/cleanup.

## Examples

`examples/embedding/os_ports/main.c` is a compilable C89 example. `example.as` is illustrative only. The current render job scheduler is not yet connected to MovieClip rasterization.

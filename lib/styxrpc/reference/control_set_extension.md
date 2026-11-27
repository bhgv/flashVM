# styxrpc_control_set_extension

[Русский](control_set_extension.ru.md) · [API](../include/styxrpc/control.h#L51) · [Implementation](../src/control.c) · [Control tree](control_create.md)

```c
void styxrpc_control_set_extension(StyxRpcControl *control,
    int (*command)(void *context, const char *bytes, size_t size));
```

Install `command` on the owner thread, or pass NULL to clear the extension.
NULL `control` has no effect. The SDK continues to handle the four standard
words first. For any other write it calls the extension with the original
control spec context and the counted command bytes, excluding one optional
trailing LF. The bytes have no required NUL terminator. They are borrowed
for the call. Return 1 to acknowledge the write. All other returns reject
it. Each command requires an independent write fid starting at offset zero.

An application can use extra commands for operations such as selecting a
debugger stop. That policy and any state referenced by the callback belong
to the application.

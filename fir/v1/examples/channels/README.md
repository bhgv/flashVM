# Channels

[Русский](README.ru.md).

`limbo.chan_create` makes an unbounded queue; `limbo.chan_send %ch, %answer`
copies a message; `limbo.chan_receive %ch` receives it; `limbo.chan_close %ch`
closes the channel. The optimiser preserves these effects. The host must call
`fvm_extensions_install(context)` before execution. This maps to the existing
extension, not a new channel implementation.
[Extension API](../../../../flashVM/doc/ACTIONSCRIPT_EXTENSIONS.md),
[channel C API](../../../../flashVM/doc/CHANNELS.md).

```sh
build/fir1/fir1 check fir/v1/examples/channels/main.fir
build/fir1/fir1 opt fir/v1/examples/channels/main.fir build/fir1/channels.fir
build/fir1/fir1 swf build/fir1/channels.fir build/fir1/channels.swf
```

Expected trace: `42`.
[CLI](../../compiler/README.md), [FIR](../../doc/FORMAT.md),
[tests](../../doc/TESTING.md). `python fir/v1/view_docs.py`.

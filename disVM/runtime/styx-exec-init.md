# Styxexec.init

[Русский](styx-exec-init.ru.md) · [Guide](STYX-EXEC.md) · [Interface](../module/styxexec.m#L12) · [Source](../limbo/styxexec.b#L15)

```limbo
init: fn();
```

Include `styxexec.m`, load the installed `styxexec.dis` and check the
module reference before calling `init()`. Initialization loads Sys.
It takes no arguments, returns no value and opens no session. Initialize
once before sharing the module with processes using [call](styx-exec-call.md).

```limbo
exec := load Styxexec Styxexec->PATH;
if (exec == nil) {
    raise "cannot load Styxexec";
}
exec->init();
```

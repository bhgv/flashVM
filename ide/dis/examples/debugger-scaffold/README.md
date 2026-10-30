# Disconnected debugger session example

[Русский](README.ru.md) · [Graphical panels](../../../DEBUGGER.md)

The commented [acceptance program](../../tests/idedebug_test.b) creates a genuine
session owner, reads its disconnected capabilities and attempts attach/pause/
continue/step/next. Each unsupported operation is explicitly refused and preserves
its snapshot. A stale Shutdown must not terminate the session; the exact current
incarnation receives its terminal acknowledgement. No target frames are invented.

Installed package commands:

```sh
./limbo-native -g -I include/limbo -o debug-check.dis examples/debugger-scaffold/acceptance.b
./disvm --stats --test-control --jit debug-check.dis
./disvm --stats --test-control --no-jit debug-check.dis
```

Expect `PASS disconnected debugger` and positive/zero native counters respectively.
For actual MovieClip panels launch the desktop and use Debug → Open panels.
This example verifies the initial session protocol, not execution debugging.

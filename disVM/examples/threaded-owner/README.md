# Whole managed application on the dedicated Dis owner

[main.b](main.b) is readable Limbo with four private computation arrays and four
blocking result-channel sends. Each task computes a distinct checked sum. The
root verifies identities, results and final array values, prints PASS and returns
normally. No process-exit test-control escape bypasses owner/TLS cleanup.

The tasks created by spawn are cooperative Dis processes inside the dedicated
Dis OS worker. Main/UI is a separate OS thread in disvm-editor. This example does
not label each Limbo task as a native thread or access shared FlashVM objects.

The workspace build creates `threaded-owner.dis`; the installed runtime ships it
beside the other modules and includes this source. From that package:

```
DVM_EDITOR_THREAD_STATS=1 ./disvm-editor --threaded-ui --jit threaded-owner.dis
DVM_EDITOR_THREAD_STATS=1 ./disvm-editor --threaded-ui --no-jit threaded-owner.dis
```

Use `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy` for headless execution. The example
creates no document scene; its role is real VM/channel/collector computation while
the UI owner stays independently scheduled. Interactive graphics are exercised by
the installed desktop IDE, not mocked inside this calculation example.

To recompile using only the matching installed SDK:

```
./limbo-native -g -I include/limbo -o threaded-owner.dis examples/threaded-owner/main.b
```

Keep the compiled entry beside the package's module resources. Imports resolve
relative to the entry directory. The expected application line is
`PASS threaded-owner: four managed channel computations, normal owner retirement`.
The runtime then reports successful completion; thread diagnostics show main
without Dis TLS and the worker's bound/released context. See the complete
[ownership/shutdown contract](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/THREADING.md).

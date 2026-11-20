# Run a Dis command by VFS path

[Русский](README.ru.md) · [Client](runexample.b) · [Interface](runexample.m) · [Tutorial](TUTORIAL.md) · [Overview](../../runtime/VFS-LAUNCH.md)

The example submits a mounted command to its owner, sends binary data through
a route configured to receive it, and reads the exit status with ordinary
`Sys->open/read` calls. The client loads `Vfsctl` through ordinary `load`.

```limbo
example := load Runexample Runexample->PATH;
payload := array[] of {byte 'x', byte 0, byte 'y'};
code := example->execute("/routed.dis", "/payload.dis", "/run-result", payload);
```

Here `/routed.dis` is the visible command name. The owner publishes its result
at `/run-result`. The separate `/payload.dis` route demonstrates an owner
callback configured to receive bytes. Pass `nil` instead of `/payload.dis`
to omit the independent payload submission.

## Run the example

From the repository root on Linux with CMake, a C compiler and Python:

```sh
cmake -S disVM/runtime -B build/launch-runtime -DDVM_RUNTIME_TESTS=OFF
cmake --build build/launch-runtime --target disvm --parallel
cmake -S disVM/native -B build/launch-sdk \
  -DINFERNO_SOURCE="$PWD/disVM/native/src" \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD" \
  -DINFERNO_DISVM_RUNTIME="$PWD/build/launch-runtime/disvm" \
  -DINFERNO_LIMBO_COMPILER="$PWD/build/launch-runtime/bootstrap/limbo-bootstrap"
cmake --build build/launch-sdk --target terminal-ordered-files --parallel
ctest --test-dir build/launch-sdk -R '^terminal-ordered-files-guest$' --output-on-failure
```

The test builds and publishes `runexample.dis` and `vfsctl.dis`, connects the
namespaces over TCP, and runs interpreter and JIT variants. A pass means
the command's `init` read the owner's file, the owner received `x\0y`, and
the client read exit status 0 over the still-open connection.

# Load a managed Dis library over Styx

[Русская версия](README.ru.md) · [Remote Dis examples](../README.md)

Two DisVM processes share one Styx session. The owner serves two exports from
one loaded instance of [target.b](target.b). The caller imports [managed.m](managed.m),
loads the mounted module path and calls both functions with ordinary Limbo syntax.
The owner builds the bridge from the types recorded by the Limbo compiler.

## Run

Build the native Styx SDK and the DisVM runtime with the same SDK. From the
repository root, give the runner the matching DisVM and Limbo compiler binaries.

```sh
python3 disVM/examples/remote-scalar/managed-fid/run.py \
    build/scalar-runtime/disvm build/scalar-runtime/bootstrap/limbo-bootstrap
```

The runner uses separate temporary directories, compiles the images with
`-D RG`, connects both processes over a single loopback TCP connection and
cleans up on exit. Expect `PASS managed Styx load` in both execution modes.

## Follow the call

1. The client's `load` matches the owner signature and schema in `.interface`
   against its compiled import. Both functions are files inside `managed.dis`.
2. The client opens one function fid and writes a complete S-record with
   `(client PID, arguments…)`. The record can span several Styx writes. Its
   encoded length tells the owner when to schedule the call.
3. The owner gives the request to the caller's worker. The generated bridge
   decodes a local copy of the arguments and calls the original export with
   `mcall`. It writes the result through the private route for that fid.
4. The client reads and closes the fid. The reply has positions for the result
   and reference updates. The result is `nil` for a void export.

The example checks a UTF-8 string, a returned byte array, a void return, an
owner exception and two parallel calls whose records exceed one Styx message.
The managed value profile supports strings, primitive arrays and tuples within
the schema and record limits. Its reference-updates position is empty. The
existing scalar `ref` profiles handle their own copy-back.

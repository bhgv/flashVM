# Generated scalar owner from a Limbo .m interface

Copyright (c) 2026 bhgv

[Русский](README.ru.md) · [Interface](compute.m) ·
[C generator](../../../compiler/limbo/frontend/remote_owner.c) ·
[Network owner contract](../../remote/NETWORK-OWNERS.md)

This example generates the owner-side bridge from an ordinary `.m`, compiles the
bridge into `.dis`, and invokes a normal implementation in another real DisVM.
The `.m` describes the interface; requests/results travel through the existing
Proxynet connection. No `.m` file transmits values and no provider bytecode is
loaded into the client VM.

The first supported profile is deliberately exact: one module with one
`fn(value: int): int` export. The parameter/export names are taken from the existing
C99 Coco/R catalog, not a regular expression or another Limbo parser. The generated
adapter uses ordinary typed `load` of the selected implementation, so the original
Dis linker checks its actual function signature. Source is compiled with the
matched native-cell producer already used by the runtime; this does not claim a
new general Dis backend.

## Build and generate

Build `compiler/limbo/frontend` with the explicit Coco/R SDK settings documented
in its [frontend guide](../../../compiler/limbo/frontend/README.md), then:

```sh
cmake --build build/remote-owner-compiler --target limbo_remote_owner
build/remote-owner-compiler/limbo_remote_owner \
  disVM/examples/remote-scalar/compute.m compute.m implementation.dis handler.b
```

| CLI argument | Meaning |
| --- | --- |
| INPUT.m | Explicit interface source, at most 2MiB |
| INCLUDE.m | Basename made available to the Dis compiler; must contain that interface |
| IMPLEMENTATION.dis | Provider-local module path chosen by the trusted launcher |
| OUTPUT.b | Generated Proxyhandler implementation; compile it on the normal Dis path |

Include/path spellings are bounded to255 ASCII bytes using letters, digits, `_`,
`-`, `.` and, only for the implementation path, `/`. Names conflicting with
adapter imports/locals refuse. Includes, ADTs, additional exports,
variadics, polymorphism, other arities and non-int argument/result types are
unsupported and fail before the destination opens. A syntax error also preserves
existing output. Write/close errors may leave an output prefix; the CLI returns2
on failure and0 on success. This is a host C tool, with no VM/network dependency.

## Execute the complete test

```sh
python3 disVM/runtime/test_generated_remote.py \
  build/remote-owner-compiler/limbo_remote_owner \
  build/native-dynamic/disvm \
  build/native-dynamic/bootstrap/limbo-bootstrap disVM/upstream/module
```

The driver owns two independent temporary directories. Only the provider receives
`implementation.dis` and `provider-secret`; the client receives the existing
connection libraries and its caller. It starts two actual VM processes over a
trusted inherited TCP stream, in interpreter/JIT and both register polling modes.

The generated handler loads the original implementation on its permanent owner,
retains that module and checks owner PID on calls. The real implementation reads
its provider-local file and increments module state across calls, including a
raised error. The client verifies copied scalar results, OwnerError, successful
recovery after the error and explicit Stop/join. Existing root/resident/RC drain
checks run for both VMs. Requests are never retried after uncertain completion.

The test's NC64 `int` uses the existing private network owner's signed scalar
representation. It does not promise interoperability with a classic32 peer or
assign new public wire fields, operation IDs or S-record tags.

## Current boundary

This delivers generated **owner dispatch**, not automatic remote `load` or a
complete client module proxy. The test client still calls the existing Proxynet
API explicitly. It does not yet marshal strings, ADTs, arbitrary argument lists,
channels or FDs; generated unsupported handler operations raise explicitly.
Implementation selection and transport admission remain host responsibilities.
No VFS root is reset, no remote tree is snapshotted, and no code/device dependency
is moved to the terminal.

Next work connects this checked interface generation to client-side module binding
and expands marshalling by supported type categories. The current profile is a
working bounded path, not a placeholder claiming arbitrary signatures.

[C API: limbo_remote_owner](../../../compiler/limbo/frontend/REMOTE-OWNER.md)

Constants, including PATH, remain source declarations and grant no transport authority. The existing C resolver resolves unqualified argument/result aliases lexically: module scope precedes file scope and each alias uses its declaration scope. Used cycles, unknown/qualified types and a non-int resolved type refuse before output. The generator does not evaluate constant expressions; the normal Dis compiler checks the whole interface. The trusted launcher still explicitly chooses the implementation path.

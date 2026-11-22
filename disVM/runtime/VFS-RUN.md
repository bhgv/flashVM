# Vfsctl.run

[Русский](VFS-RUN.ru.md) · [Overview](VFS-LAUNCH.md) · [Tutorial](../examples/vfs-run/TUTORIAL.md) · [Source](vfs/vfsnative.c)

## Synopsis

```limbo
include "vfsctl.m";
# Load Vfsctl and call its init() before using run.
run: fn(path: string): int;
```

Available in hosted DisVM with [Vfsctl](../module/vfsctl.m) and the `$Vfsnative`
builtin. `path` is a required, nonempty VFS name of at most 4095 UTF-8 bytes.
Absolute paths, relative paths and names created by bind are accepted. Relative
paths use the calling process's namespace and working directory.

## Result and effects

| Result | Meaning |
| --- | --- |
| `1` | Owner confirmed success, or a delivery-only route acknowledged the request. |
| `0` | Invalid path, unavailable route, or request refused before confirmed delivery. |
| `-1` | Delivery or completion is uncertain. |
| `-2` | The owner confirmed execution failure. |

The selected provider supplies the route. The call submits one empty payload
and releases its temporary route and namespace references on return. It can
wait for provider I/O. The runtime provides a cancellation scope and preserves
the reported delivery outcome if cancellation races the write. The operation
does not retry. The host controls execution and the lifetime of any result file.

A scoped Styx route can open `.unibus` for reading and writing and keep the
same fid through completion. The caller receives the status directly from
this operation.

## Example

```limbo
delivery := vfs->run("/routed.dis");
```

For a legacy delivery-only route, a result of 1 permits reading the separately
published result file. See the [TCP example](../examples/vfs-run/runexample.b) and
[`runargs`](VFS-RUNARGS.md).

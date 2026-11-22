# Vfsexport.subtree

[Русский](VFSEXPORT-SUBTREE.ru.md)

Capture one node from a VFS capability so it can be mapped elsewhere.
Opening a local mount for the source is optional.

```limbo
include "vfsexport.m";
exports := load Vfsexport Vfsexport->PATH;
pipe := exports->recordpipe();
sender := exports->subtree(pipe, "send");
```

`subtree(root, relative)` borrows an existing `ref Vfsexport->Root`.
Pass a relative path within that capability. A single dot names the root.
Absolute paths, parent traversal and empty path components are refused.
The result is an owned `ref Vfsexport->Root` with the captured node's
access. The result can outlive the original reference. Cancellation,
missing nodes and capture failures return `nil`.

Capture can wait for the source provider. It follows the existing native
cancellation contract. The call only captures a node. To publish it,
pass the result to `Vfsexport->overlay` with the desired target and
access ceiling.

The [record pipe example](RECORD-PIPE.md) publishes the captured end as
a numbered file in the caller's Styx descriptor grant.
[Limbo interface](../module/vfsexport.m).
[Native implementation](vfs/cap_tree.c).

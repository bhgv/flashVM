# Vfsnative.scalarload

[Русский](VFS-SCALAR-LOAD.ru.md) · [Interface](../module/vfsnative.m) · [Service](SCALAR-OWNER-SERVICE.md)

```limbo
scalarload: fn(path, exportname: string): Scalarimplementation;
```

Use the hosted `$Vfsnative` builtin. `path` names the original Dis image in
the active VFS. `exportname` is a nonempty name of at most 48 bytes. The
ordinary Dis reader loads the image. The ordinary linker binds the named
export with signature `fn(int): int` to the single `invoke` slot. It returns
an owned module reference which follows ordinary Limbo lifetime rules.

The call may block in the loader. Malformed arguments raise an exception.
An unavailable image, native gateway image, missing export or signature
mismatch returns nil. The builtin binds the implementation without executing
the export. The caller invokes it through `target->invoke(value)`.

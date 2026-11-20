# Vfsctl.run

[Русский](VFS-RUN.ru.md) · [Limbo interface](../module/vfsctl.m) · [Runtime](vfs/vfsnative.c)

`run: fn(path: string): int` submits a program through the route advertised
by its selected VFS provider. Initialize `Vfsctl` once and pass the visible
absolute or relative path, including a bound alias.

```limbo
status := vfs->run("/mounted/app.dis");
```

Returns 1 when delivery is acknowledged, 0 on refusal or an unavailable route,
and -1 when delivery is uncertain. The operation can wait for Styx. It sends
an empty startup payload and performs no automatic retry. Delivery acknowledges
the request, while starting the process belongs to the owner.

The TCP guest test submits a bound alias for a separate command image. The
fixture owner starts the command when it accepts the request. Its `init` reads
a file in the owner's working directory. The requesting guest then reads the
fixture's completion file over the same live mount. The check runs in
interpreter and JIT.

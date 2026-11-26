# Node.refuse

[Русский](styx-exec-refuse.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L36) · [Implementation](../limbo/styxexec.b#L168)

```limbo
node.refuse(request: ref Request): int
```

Refuse a Request from this Node without sending result bytes. Return 1 when accepted or 0 for a stale, foreign, closed or already answered request. The peer observes an unsuccessful invocation. A successful empty reply is submitted through reply.

```limbo
refused := node.refuse(item);
```

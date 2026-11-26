# Node.reply

[Русский](styx-exec-reply.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L33) · [Implementation](../limbo/styxexec.b#L161)

```limbo
node.reply(request: ref Request, bytes: array of byte): int
```

Submit bytes for a Request received on this Node. A reply length may be zero and may not exceed the input byte count. The host copies the bytes. Return 1 if acknowledged, 0 on invalid arguments, expiry, duplicate answer or a local write failure.

```limbo
accepted := node.reply(item, item.bytes);
```

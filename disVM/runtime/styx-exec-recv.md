# Node.recv

[Русский](styx-exec-recv.ru.md) · [Guide](STYX-EXEC-PUBLISH.md) · [Interface](../module/styxexec.m#L30) · [Implementation](../limbo/styxexec.b#L123)

```limbo
node.recv(): ref Request
```

Wait in the calling Limbo thread until this published Node has a complete request. Return a copied Request with owned argument bytes and its exact ticket. Use one receiver per Node and keep processing through reply or refusal. The call raises if the local owner descriptor is unavailable.

```limbo
item := node.recv();
```

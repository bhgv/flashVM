# styxrpc.publishObject

[Русский](STYX_RPC_PUBLISH_OBJECT.ru.md) · [Player](STYX_RPC_PLAYER.md) · [Implementation](../src/styxrpc/player_services.c)

```actionscript
receiver.total = 41;
styxrpc.publishObject("sample.dis", receiver, ["update", "apply"],
    ["total", "total", "var", "rw"]);
```

Pass a single basename and a live receiver. The published image is available
to the peer at `mounted/<basename>`. Supply one to eight methods and up to
eight Int32 variables in any order.

| Fields | Meaning |
| --- | --- |
| `[export, member]` | Dispatch a `fn(int): int` callback on the receiver. |
| `[export, member, signature, arguments, result]` | Dispatch a managed callback whose compiled Dis signature and schemas match this descriptor. |
| `[export, member, signature, arguments, result, "graph"]` | Dispatch through the existing graph codec for nested and returned references. |
| `[name, property, "var", "ro"]` or `[name, property, "var", "rw"]` | Publish the live Int32 receiver property at `.var/<name>` with the given access. |

The call returns true after admitting the image. Invalid descriptors and
duplicate publication raise an error. The player retains the receiver and
calls its methods on the owner thread. A movie may publish one image. The
player withdraws it when the movie is replaced. Old fids fail on subsequent
access. See [loadModule](STYX_RPC_LOAD_MODULE.md) and [intVariable](STYX_RPC_INT_VARIABLE.md).

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
| `[export, member, signature, arguments, result, "channel", slots]` | Call a factory with value arguments and one typed channel result, such as `"(i)"` and `"(c[i])"`. The factory can return an initial element for the result queue. `slots` is an integer from 0 (rendezvous) to 8. |
| `[name, property, "var", "ro"]` or `[name, property, "var", "rw"]` | Publish the live Int32 receiver property at `.var/<name>` with the given access. |

The call returns true after admitting the image. Invalid descriptors and
duplicate publication raise an error. The player retains the receiver and
calls its methods on the owner thread. A movie may publish one image. The
player withdraws it when the movie is replaced. Old fids fail on subsequent
access. See [loadModule](STYX_RPC_LOAD_MODULE.md) and [intVariable](STYX_RPC_INT_VARIABLE.md).

Channel factories use the same [module lease registry](../../lib/styxrpc/reference/module_channels_init.md)
as C publishers. Up to 16 leased result families can be live per movie.
An ordinary typed Limbo `load` can call the factory and use the returned
channel. The calling function fid holds the lease and its close or movie
replacement withdraws it. The stock AS importer still needs a configured
reverse caller PID to load a channel-valued export.

The paired `--module-channel` [test](../../lib/styxrpc/tests/flashvm/mixed_pair.py)
compiles the [AS publisher](../../lib/styxrpc/tests/flashvm/module-channel_pair.as)
and the [Limbo consumer](../../lib/styxrpc/tests/flashvm/module-channel_pair.b)
with project tools and exercises two independent queues, a scalar call and
live receiver data.

# vfs.unload

[Русский](STYX_RPC_UNLOAD.ru.md) · [File methods](STYX_RPC_OS.md) ·
[Implementation](../src/styxrpc/module_import.c)

```actionscript
vfs.unload(module)
os.unload(module)
```

Close a checked Dis proxy in the stock paired player, from the film or
the detached task that owns it. Pass the object returned by
[`file.load()`](STYX_RPC_OS.md) or a checked module loader in this
context. Other objects and argument counts raise an argument error.

The first call returns `true`. A later call on the same proxy returns
`false`. All aliases and already extracted methods share this state.
A new method call through the closed proxy raises an exception. If a
getter closes the proxy during argument conversion, the invocation that
entered before closure can finish with its original fid.

The proxy releases its directory fid and its share of the live module
lease. Returned channels hold their own share. They remain usable after
unload and release that share when closed or collected. The last holder
revokes the generation on DisVM. This method can wait for Styx clunk
acknowledgements. The connected peer retires its resident and recycles
the slot asynchronously.

```actionscript
var source = os.open("/library/counter.dis");
var counter = source.load();
source.close();
var answer = counter.next(41);
var released = os.unload(counter);
```

Closing the source leaves the proxy usable for the call. After unload,
`released` is `true`. A fresh load obtains its own proxy. See
[live loading](STYX_RPC_LIVE_DIS.md) and
[returned channel objects](STYX_RPC_CHANNELS.md#use-disvm-channels-from-actionscript).

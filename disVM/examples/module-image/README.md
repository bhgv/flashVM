# Acquire a module and send it through FlashVM

[Русский](README.ru.md) · [Host API and build](../../host/README.md) ·
[Fully commented source](main.c)

This native example copies an existing `.dis` file through FlashVM's portable
file adapter, sends arbitrary bytes through an actual FlashVM channel, receives
an independent copy and compares every byte. Embedded NUL/non-WTF-8 bytes are
preserved by the explicit `dis-hex:` envelope. It does not run the module or prove
Dis interpreter/JIT execution.

Build the shared host with an actual FlashVM target/library and its dependencies
as explained in the host guide. The optional `disvm_module_image_example` target
is enabled with `DVM_FLASHVM_LIBRARIES`, alongside the channel integration test.
Supply a real compiled module and its exact size:

```sh
build/disvm-host/disvm_module_image_example build/workspace/ide/idecore.dis BYTE_COUNT
```

Replace `BYTE_COUNT` with the file's byte length from your file manager or resource
metadata. A successful run prints `PASS acquired and transferred ... module bytes;
no VM execution`. File errors, truncation, quotas, allocation or channel errors
return nonzero. Caller-supplied metadata frames exactly one image; trailing bytes
are not read. This demonstrates the input boundary used by disk, IDE buffers and
embedded resources, rather than a filesystem namespace resolver.

Both images own native allocations; no VM heap pointers cross the channel. The
channel remains buffered because this one-thread tutorial sends before receiving.
For rendezvous, place producer and consumer on separate native threads and use
`fvm_channel_set_rendezvous`. Every image/channel is released in the common cleanup
path. A deployed IDE will be Limbo/AS bytecode, not this native demonstration.

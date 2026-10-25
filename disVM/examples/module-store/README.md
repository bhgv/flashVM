# Publish and pin a compiled module

[Every-line source](main.c) · [Namespace contract](../../MODULE-STORE.md)

The example reads a real `.dis` image through FlashVM's portable file adapter,
publishes an independent copy, releases its source, pins the published version,
and tears down both owners. Every code line is commented. It does not execute
Dis bytecode or require another Inferno installation.

Build the `disvm-host` component using the normal root launcher. The executable
`disvm_module_store_example` and this source are installed into its `out` package.
Pass an existing module plus its exact framed byte length:

```sh
./disvm_module_store_example ../ide/modules/ideworkspace.dis BYTE_COUNT
```

The number is the file's actual size from your file manager/resource metadata.
A successful run reports the byte extent and generation, explicitly stating
`no VM execution`. Missing/truncated files, invalid lengths, quota failures and
unbalanced ownership return nonzero. Trailing file data is not read beyond the
supplied frame. Input bytecode validity remains the real Dis parser's job.

Для запуска нужен собранный `.dis`, например ideworkspace.dis из пакета IDE,
и его точная длина в байтах. Пример показывает владение и версии в памяти;
успешное хранение не означает успешную проверку или исполнение байткода.

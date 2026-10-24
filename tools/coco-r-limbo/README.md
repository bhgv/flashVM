# Selected Coco/R C/Limbo generators

[Русский](README.ru.md) · [Provenance](../UPSTREAMS.md)

Source: https://github.com/bhgv/coco-r-limbo, master; revision and file hashes:
[UPSTREAM.json](UPSTREAM.json). The C program emits C by default and Limbo with
--limbo. The native Limbo generator source is upstream/limbo/cocor.b; its C/Limbo
runtime fragments are required generation inputs, retained beside it.
No examples, precompiled Dis files or external Inferno installation are bundled.

```sh
./build-workspace.sh --projects cocor-limbo --tests
```

This builds the C generator and cocor.dis using the source-built local bootstrap.
For standalone C-only generation, configure this directory directly with CMake.
Supply LIMBO_EXECUTABLE and LIMBO_MODULE_DIR explicitly to also build cocor.dis.
Execute Dis products only in a runtime with a compatible module/native-frame ABI.
The partial disVM core archive is not yet a standalone cocor.dis launcher.
The source generator's Draw reference is its conventional Context signature;
this build links no Draw/Tk implementation. Keep runtime fragments with the
source generator; pass its runtime directory according to upstream's -r option.

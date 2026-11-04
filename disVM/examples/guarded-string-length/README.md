# Guarded ASCII and Rune string length

`main.b` is a fully commented runnable Limbo example. It grows an ASCII and a
Unicode string, checks32 logical lengths, and checks the normal empty string.
UTF-8 byte counts do not define the logical Rune count. Internal String.len uses
signed int; native WORD output may be wider. LENC reads exactly the int field,
normalizes a negative Rune count, and rejects INT_MIN before output publication.

From the repository root, set TMPDIR to an existing writable build directory:

```sh
build/install1801/disvm-editor/limbo-native -F -I disVM/upstream/module -o build/string-length.dis disVM/examples/guarded-string-length/main.b
build/workspace1735/disvm-editor/disvm --gc-worker --no-jit build/string-length.dis
build/workspace1735/disvm-editor/disvm --gc-worker --guarded-copies --no-jit build/string-length.dis
build/workspace1735/disvm-editor/disvm --gc-worker --guarded-copies --jit build/string-length.dis
```

From the installed SDK root, use `./limbo-native -F -I include` with
`examples/guarded-string-length/main.b`, then the same three `./disvm` profiles.
Expected:`PASS string length32`. Physical ARM64/RV64 execution is not implied.

Source/output guards remain live while the discovered int field is added.
Caller-owned referent/root lifetimes and collector exclusion remain independent;
no temporary root or allocation is introduced. H gives0, internal NULL exNilref,
invalid geometry/INT_MIN exHeap. All errors leave output unchanged and release
all guards. General managed code still uses NativeWorld. See
[the contract](../../runtime/STRING-LENGTH.md), installed at SDK root.

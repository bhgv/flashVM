# Native C interfaces from .m — our Coco/R compiler

[Русский](NATIVE_C.ru.md) · [API](native.h) · [Emitter](native.c) ·
[Tests](tests/native_check.py) · [Runnable tutorial](../../../disVM/examples/native-module/README.md)

`limbo_bindings --format native-c-nc64 INPUT.m OUTPUT.h` emits native Dis C
interfaces. `native-c-classic32` explicitly selects the classic four-byte-cell ABI.
Neither mode invokes the bootstrap compiler. They consume the owned interface
catalog built by our Coco/R grammar, then resolve the supported primitive types and their unqualified scalar aliases, compute layouts/signatures/maps and emit checked C. The library entry is
`limbo_native_header` in target `limbo_interfaces`; output NULL performs preflight.

The existing `--format c` remains a value/descriptor external-call scaffold; its
FNV `interface_id` is not a native Dis signature. The new modes emit real structural
MD5 signatures folded like Inferno `sign()`, a distinct contract. MD5 here identifies
ABI structure, not trusted peers or authenticated code.

## Generated interface

- `F_Module_export` structures with register/return/scratch prefix, explicit target
  padding and typed parameter fields.
- Native `void Module_export(void *frame)` prototypes.
- `Modulemodtab`/`Modulemodlen`, including names, actual structural signatures,
  eight-byte-rounded descriptor sizes and 16-byte-capacity pointer maps.
- Compile-time checks for host pointer/WORD/LONG/REAL sizes, NREG, frame extents and
  every argument offset. Wrong-host classic32 headers fail when compiled on NC64.

String **inputs** mark their argument slots; the return pointer itself is not a
managed input. Target offset slots, not `sizeof(void*)` on the generator host,
determine map bits. Result values are published into caller-owned return storage.
Void exports carry `noret`, never a fabricated result pointer.

Supported now: one top-level module, 1–128 exports, 0–16 arguments each, direct
byte/int/big/real/string or their scalar aliases, scalar or void result, ASCII
C-safe export identifiers <=63 bytes.
Module constants are tolerated but are not emitted as C defines. Nil parameters
receive generated argument names; collisions and C/reserved frame names fail.
Includes, ADTs, composite types, self, varargs, polymorphic or raises
signatures are rejected before destination opening. No approximate metadata is
produced for a type that needs unresolved layout or signature recursion.

The library prevalidates before writing. It owns temporary metadata only and
retains no catalog/source pointer after return. Independent emission calls may
read immutable catalogs concurrently; coordinate catalog destruction and shared
streams. Parsing still uses the existing serialized Coco/R API. Stream I/O or
allocation failures are returned; callers own publication/rollback policy.

## Evidence

The regression independently calculates frame alignments/maps and uses Python
hashlib for the folded MD5. With explicit oracle compiler paths it additionally
compares against selected Inferno classic32 and NC64 output. It compiles the NC64
header in C99 and verifies that the wrong host is rejected. Unsupported inputs
preserve an existing destination. The runtime tutorial is built using **our**
header and executes real Limbo/native calls, GC and errors on our DisVM.

```sh
# Optional additional oracle comparison; oracle paths are explicit development inputs.
python3 compiler/limbo/frontend/tests/native_check.py \
  build/limbo-own/frontend/limbo_bindings \
  /absolute/classic/limbo-bootstrap /absolute/nc64/limbo-bootstrap
```

This does not yet replace generated interfaces for complex Sys/ADT/channel modules.
The matched bootstrap remains the temporary full-module compiler for this runtime;
its classic/NC64 headers must never be interchanged. Extend this semantic backend
through authoritative type resolution and independent ABI tests, not another
handwritten parser or guessed C structures.

Generated callback, frame typedef, size/offset check and registration names are
checked together before output opens. Collisions (including native_abi, NativeExport
or add/add_frame_size) preserve the old file; suffix names alone remain legal.

Scalar aliases in module/file scope now resolve through the [binding pass](ALIASES.md).
Qualified/generic/composite aliases remain outside this native profile. Source
alias names do not become C typedefs or alter frame/signature/map metadata.

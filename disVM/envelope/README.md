# Dis64 structural codecs and inspector

[Русский](README.ru.md) · [Commented C99 example](examples/main.c) ·
[Envelope contract](../upstream/doc/dis64-envelope.md) ·
[String-pool contract](../upstream/doc/dis64-strings.md) ·
[Accepted registry](../upstream/doc/dis64-registry.json)

`dis64inspect` reads the agreed Dis64 header, mandatory-feature manifest and
section directory. Its JSON explicitly says `envelope_only:true` and
`execution_supported:false`. A successful report does not mean the module,
types, code or imports are semantically executable. Old NC64/Dis files are
not converted. The utility rejects mandatory VM extensions it does not implement.

The C99 library interface is the same selected header used by the inspector:
[dis64.h](../upstream/include/dis64.h),
[dis-string-pool.h](../upstream/include/dis-string-pool.h) and
[dis-rune.h](../upstream/include/dis-rune.h). It is header-only and allocates
nothing; callers supply accessible stable buffers, separate output and explicit
resource limits. Borrowed views expire when their immutable input expires.
No VM heap, native pointer transport, frame tick or interpreter thread is used.

The [fully commented example](examples/main.c) constructs a valid structural
container through those interfaces. M/C/T are opaque fixture markers, not a
program or real type/module descriptors. Its strings section is real: two
separate IDs contain the same full-bit Rune value. It demonstrates write/read,
lookup and signed content comparison without normalizing or interning strings.

## Source build

```sh
cmake -S disVM/envelope -B build/envelope
cmake --build build/envelope
ctest --test-dir build/envelope --output-on-failure
build/envelope/dis64inspect build/envelope/sample.dis64
build/envelope/dis64inspect build/envelope/sample.dis64 --rewrite build/envelope/copy.dis64
```

The standalone build needs C99 and CMake; Python is used only for host tests.
Core/runtime builds also build the same tool and sample. There is no implicit
bootstrap Limbo, Coco/R generator, FlashVM, SDL or whole-OS dependency here.
For the inspector alone, the upstream source also has a small Makefile.

## Installed SDK

From the installed `disvm-editor`, `disvm-runtime`, `disvm-host` or `disvm` package:

```sh
./run-dis64-envelope.sh
./dis64inspect examples/dis64-envelope/sample.dis64
mkdir -p work
cc -std=c99 -Iinclude/dis64 examples/dis64-envelope/main.c -o work/envelope-example
./work/envelope-example work/sample.dis64
./dis64inspect work/sample.dis64 --rewrite work/copy.dis64
```

The launcher inspects the packaged fixture and requires no source checkout.
Build-menu lists it as a separate **Dis64 envelope demo** when installed.
It does not appear as a new executable DisVM program or as an IDE debugger.
The C example stages all validation before opening its explicit output filename.
Final file I/O is checked but is not an atomic filesystem transaction.

## Implementation and acceptance boundaries

Envelope codec: exact version/ABI/flags, sorted feature names, feature policy,
dense extents, singletons, known required kinds and caller limits. String pool:
count+non-nil Rune records, exact consumption, ordinal IDs, duplicate contents,
signed32 comparisons and atomic rejection. Lookup currently scans to an ID;
a future loader may build an independent checked offset index.

Tests run the actual built inspector and a freshly linked C99 client, compare
canonical bytes/reports, reject malformed input before rewriting a destination,
and keep the non-execution status explicit. Upstream fixtures independently
exercise the codec and wire-pool-to-typed-case composition. SDK tests relocate
products and rebuild the example from shipped headers only.

Module/type/code/import payloads and the first executable pilot remain in the
[direct payload exchange](../upstream/doc/dis64-flashvm-payload-response.md).
That proposal is not implemented by these structural tools. No new runtime ABI,
ref-result GC/unwind acceptance or physical ARM64 execution is claimed here.

Native builds generate the sample through the C99 client. Cross builds do not
execute target binaries: the installer decodes the verified sample.dis64.hex
fixture using its existing host Python dependency. The deployed demo requires
only its native inspector and fixture, not Python. Cross-platform runtime
execution is separate acceptance.

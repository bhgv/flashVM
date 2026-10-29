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

## Real payloads: explicit scalar pilot

The [line-commented pilot example](examples/pilot.c) constructs genuine module
metadata, complete type maps, code, default initialization and a name string pool.
Its 180 bytes match the independently hand-written
[pilot42 golden](../upstream/libinterp/tests/dis64/pilot42.hex). The earlier
`main.c` and sample.dis64 remain opaque structural fixtures for compatibility.

```sh
build/envelope/dis64_pilot_example build/pilot42.dis64
build/envelope/dis64inspect build/pilot42.dis64 --pilot
build/envelope/dis64inspect build/pilot42.dis64 --pilot --rewrite build/checked.dis64
```

`--pilot` uses the actual shared [semantic checker](../upstream/include/dis64-pilot.h).
It checks ordinal name/type IDs, data extent, entry/frame, complete scalar maps,
code count, instruction modes/width/alignment/bounds and supported section policy.
Its JSON says `scalar_pilot:true`, `envelope_only:false`, and still
**`execution_supported:false`**. Unsupported instructions/imports/initializers
reject; a valid envelope alone does not bypass this semantic gate. See the
[exact upstream subset](../upstream/doc/dis64-pilot.md).

The installed SDK includes `run-dis64-pilot.sh`, `pilot.c`, `pilot42.hex`,
`pilot42.dis64`, every transitive header (including isa.h once), and inspector
source. Rebuild outside the repository:

```sh
cc -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Iinclude/dis64 \
  examples/dis64-envelope/pilot.c -o work/pilot-example
./work/pilot-example work/pilot42.dis64
./dis64inspect work/pilot42.dis64 --pilot
./run-dis64-pilot.sh
```

Unused-function suppression covers the shared static header-only entry points;
other warnings remain enabled. Both native generated samples have host-side
hexadecimal installer fallbacks, so cross configuration never runs target tools.
Build-menu entry **14**, `--launch pilot`, selects this semantic demo. Entry13
keeps the structural demo. Neither launcher invokes a Dis64 VM.

The golden code is MOVW #42, MP[0]; EXIT. Upstream acceptance decodes the whole
file and separately executes MOVW through production C and AMD64 RX handlers.
EXIT is only decoded there; production module allocation, entry/EXIT scheduler,
reference-result GC/unwind and physical ARM64 are future acceptance gates.

## Scalar storage and interfaces

`dis64_pilot_example output.dis64 --initialized` now builds the shared192-byte
[init40/ADDW2 golden](../upstream/libinterp/tests/dis64/pilot-init42.hex), while
the default argv still builds the original180-byte MOVW42. The initializer is
written through dis64initializerswrite; the whole semantic gate checks it before
output is opened. Inspector JSON adds `initializer_count`:0 or1 for these fixtures.
This is a checked record count, not VM heap publication or execution evidence.

Build-menu **15**, `--launch storage`, runs installed `run-dis64-storage.sh` on
the initialized sample. Both examples have checked host hex installer fallbacks.
All four new shared headers—scalar/data/signature/bindings—ship together with the
previous closure. Source, source-built inspector and C99 clients rebuild from a
relocated package with no original source paths or runtime Python dependency.

The [commented interface example](examples/interfaces.c) uses actual versioned
scalar signatures and the binding writer/reader. Two ordinal aliases for the
same module/symbol with an exact signature are accepted; a conflicting result
kind rejects before changing any byte or written length. The example resolves
no native function and invokes no callee. Run `dis64_interfaces_example`, or:

```sh
cc -std=c99 -Wall -Wextra -Werror -Wno-unused-function -Iinclude/dis64 \
  examples/dis64-envelope/interfaces.c -o work/interfaces
./work/interfaces
./dis64_pilot_example work/initialized.dis64 --initialized
./dis64inspect work/initialized.dis64 --pilot
./run-dis64-storage.sh
```

[Upstream storage contract](../upstream/doc/dis64-storage.md) details exact bits,
no-overlap/ref-slot checks, supplied runtime nil for unpublished native storage,
versioned signatures and content-based ambiguity. Structural imports/exports
checks are not dependency resolution, argument layout, safepoint roots or a call
ABI; pilot still rejects bindings sections. Production WORD32 build and actual
allocation/entry/IEXIT->error/unwind/scheduler remain future gates. Upstream
native acceptance initializes40 and executes actual ADDW C/RX to42 separately;
EXIT is only decoded, and no physical ARM64 execution is claimed.

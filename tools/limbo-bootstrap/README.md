# Temporary source-built native Limbo bootstrap

[Русский](README.ru.md) · [Own compiler priority](../../compiler/limbo/OWN_COMPILER.md) ·
[Update policy](../UPSTREAMS.md)

Source: https://github.com/bhgv/Inferno-OS-bhgv, branch experiment. Exact commit
and imported hashes are in [UPSTREAM.json](UPSTREAM.json). Both limbo/ C sources
and appl/cmd/limbo/ Limbo sources are retained. The latter, including stubs.b and
sbl.b, are reference material for our compiler, not silently presented as our
Coco/R implementation. The primary compiler remains compiler/limbo + FIR.

Why a native tool still exists: our current numeric Limbo→FIR compiler cannot
compile the whole IDE/Coco/R generator yet. This locally built bootstrap closes
that build dependency without a ready-made foreign SDK. It is explicit and
replaceable, not the deployed IDE runtime. Its historical generated yacc parser
is compiled as source; no yacc executable is required. Our new parsers use Coco/R.

```sh
./build-workspace.sh --projects limbo-bootstrap --tests
./build-workspace.sh --projects ide
```

The selected closure includes lib9 formatting/UTF, buffered I/O, numeric conversion,
module interfaces and two MD5 implementation files for Limbo type fingerprints.
The latter is the RSA Data Security, Inc. MD5 Message-Digest Algorithm, with its
original notice retained. There is no external crypto SDK, Keyring service,
graphical implementation, whole Inferno OS or prebuilt executable dependency.
POSIX C/fenv adapters stay outside pristine upstream sources. The ARM-only
return-address inline is replaced at the include boundary by a native compiler
intrinsic; ARM assembly is not built. The bootstrap masks hardware FP traps.
This support is not a full portable Dis host/FPU/scheduler implementation.

## Products and compatibility

The tests compile unsigned Dis XMAGIC bytecode with -g, require .sbl 2.1 and
exercise legacy -a C scaffolds. Full reference flags live in the retained source;
-g emits Dis debug data, -a emits C declarations and -t MODULE emits runtime tables.
For example, after creating build/output:

```sh
build/workspace/limbo-bootstrap/limbo-bootstrap -g -I ide/dis -o build/output/idecore.dis ide/dis/idecore.b
build/workspace/limbo-bootstrap/limbo-bootstrap -a ide/dis/idecore.m > build/output/legacy-ide-declarations.h
```

Successful compilation/header/source-map checks are not VM execution acceptance.
The upstream legacy compiler emits its existing 32-bit Dis data/frame conventions;
new 64-bit JIT leaf support alone does not finish all native frame/layout ports.
A compiler and emulator from different forks must not be assumed ABI-compatible.
IDE execution tests/demo still require an explicitly selected matching EMU_EXECUTABLE;
when using another runtime, provide its matching compiler/header overrides too.

## Direct build with mk, Make or Meson

```sh
./build-workspace.sh --backend mk --projects mk,iyacc,limbo-bootstrap --tests
```

This builds actual native mk and iyacc, regenerates `limbo.y`, compiles native
Limbo with CC/ar, then tests all three products. Neither CMake nor emu is needed.
The same selection works with `--backend make` and `--backend meson`; the latter
requires Meson/Ninja. `bootstrap.sh` uses the explicit `*.sources` lists, mirrors
the standalone CMake archive/link order and writes only into the output tree.
`CC`, `AR`, `CPPFLAGS`, `CFLAGS`, `LDFLAGS` follow the other bundled native tools.
Direct invocation requires an already built matching iyacc and skeleton root:

```sh
IYACC_EXECUTABLE="$PWD/build/workspace/iyacc/iyacc" \
IYACC_ROOT="$PWD/build/workspace/iyacc" \
  bash tools/limbo-bootstrap/bootstrap.sh build/workspace/limbo-bootstrap
```

No runtime modules or whole Inferno installation are imported for this path.


## LP64 big-constant identity fix

The pinned C compiler uses a 32-byte internal name for `big` constants. On LP64,
casting a negative value/high half to `long` can format sixteen digits per half;
truncation then merges different values, for example -2 and -5. Both build routes
now prepare a derived ecom source with explicitly masked 32-bit unsigned halves,
producing a complete fixed 20-character name (21 bytes including NUL). The pinned snapshot stays unchanged.
See [preparation](prepare_ecom.py), [actual compiler-output regression](tests/big_constants.py)
and [upstream patch](patches/0001-bound-big-constant-names-on-lp64.patch).
Python 3 is a build-time source preparation dependency, not a deployed runtime.
The patch applies to the owner repository's `limbo/ecom.c`; its Limbo implementation
already uses `bhex(value,16)`. Native execution tests additionally distinguish
negative seek offsets and offsets above 2 GiB in both JIT/interpreter modes.

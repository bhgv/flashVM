# IDE compiler product admission, 0.2.8

[Русский](PRODUCT_ADMISSION.ru.md) · [Build](README.md) · [Evidence](VALIDATION.md)

A compiler can exit successfully while leaving a missing or malformed product.
The host build must reject that result before installation. The independent
[CMake guard](compile_module.cmake) removes old products, runs the explicitly
selected compiler and checks both resulting files. Any detected failure removes
both products. Keep installed releases outside this private build directory.

The module must begin with `c0 0c 80 30`: the unsigned Dis XMAGIC value 819248
encoded by `discon`. Signed SMAGIC modules are outside this compiler-product
contract. The symbols must begin with the exact physical lines `limbo .sbl 2.1`
and `Idecore`, each terminated by LF or CRLF. Empty lines, leading/embedded NUL,
extra spaces and module-name suffixes are rejected. Fixed reads of four and 25
bytes avoid scanning the remaining file just to recognize these signatures.

The wire constants are taken from the selected experiment at
[`c15785eb`](https://github.com/bhgv/Inferno-OS-bhgv/commit/c15785ebcf3611457ab72737f354bd806dc44780):
[isa.h](https://github.com/bhgv/Inferno-OS-bhgv/blob/c15785ebcf3611457ab72737f354bd806dc44780/include/isa.h)
and [discon](https://github.com/bhgv/Inferno-OS-bhgv/blob/c15785ebcf3611457ab72737f354bd806dc44780/limbo/dis.c).
No upstream source is imported by this change.

## Run the host regression without a Limbo toolchain

From the workspace root, with Python 3 and CMake available:

```sh
# Execute process doubles against the real CMake admission script.
# The first argument selects CMake explicitly; no compiler/runtime is downloaded.
# All temporary products belong to an isolated directory removed after the run.
python3 ide/dis/tests/build_guard.py "$(command -v cmake)" \
  "$PWD/ide/dis/compile_module.cmake"
```

Expected output: `46 compiler product-admission contracts passed; no bytecode validity claim`.
The same check is registered as `ide_dis_compiler_products` in CTest when
BUILD_TESTING is enabled. The standalone invocation needs no configured build.
For a detached copy, replace the two IDE paths with paths inside that copy.

The fixtures cover failed exit with outputs, missing/stale outputs, partial and
empty output pairs, arbitrary/signed/truncated module signatures, malformed and
every truncated LF symbol prefix, all four LF/CRLF combinations and opaque tails.
Successful fixtures deliberately contain only signatures or arbitrary tails;
they must never be distributed or executed as Dis modules.

## Scope

This is signature admission, not a binary parser. Full Dis validation and the
Coco/R source-map reader remain separate layers. These checks cannot detect
malformed instructions, an unrelated module with the same magic, or symbols
from a different build bearing the same module name. They provide no module
identity, cryptographic binding, debugger control or native JIT acceptance.
A valid signature alone does not make untrusted bytecode safe to load.
The Limbo application, channel schema and deployed runtime dependencies are unchanged.

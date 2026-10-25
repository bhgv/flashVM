# Real decimal conversion and Unicode append on the embedded Dis VM

[Русский](README.ru.md) · [Fully commented source](main.b) ·
[Runtime guide](../../runtime/README.md) ·
[Native-path contract](../../upstream/libinterp/tests/README.real-decimal-append.md).

This runnable Limbo example executes actual string-to-real conversion, observes
negative zero through Math.realbits64 and appends ASCII, Euro and a non-BMP
scalar while retaining the original string. It contains independent expected
values and fails before printing PASS if conversion/ownership is incorrect.
No Python, system Inferno installation or host-side numeric parser is deployed.

Build `disvm-runtime` or `disvm-editor` with the workspace menu or the documented
native CMake inputs. The source-built temporary Limbo bootstrap emits the matching
NC64 module; it is build tooling, not a replacement runtime. Our compiler/parser
work remains the Coco/R Limbo/FIR pipeline. In an installed runtime/editor package:

```sh
./disvm --stats --jit decimal-append.dis
./disvm --stats --no-jit decimal-append.dis

# Recompile solely with the installed compiler and .m headers:
./limbo-native -g -I include/limbo -o decimal-append.dis examples/decimal-append/main.b
./disvm --stats --jit decimal-append.dis
```

Output includes computed conversions of 123456789, -42, -0, 1.25 and 1e2.
The sign flag is 1 only for -0. The final retained string is `AB`, the appended
string is `ABC€😀`, and its Limbo scalar length is 5. Terminal font rendering
may vary; the bytecode checks the real Unicode values before printing.

The upstream AMD64/ARM64 CVTCF leaf accepts exact short decimal integer strings;
decimal fractions/exponents use C fallback. Appending may reuse unique storage
with spare capacity, while the retained owner/promotion in this tutorial can
require the C path. `native>0` proves real guarded JIT dispatch occurred somewhere
in the run; it does not prove every demonstrated operation ran natively.
Independent native and instruction-emulation fixtures prove those leaf paths.

The package uses experimental NC64 magic/cells and must use matching compiler,
headers and runtime. This is executable Linux AMD64 acceptance, not a complete
classic ABI or physical ARM64/RV64 runtime port. Public interfaces contain copied
strings/numbers; no heap pointer is exchanged with FlashVM.

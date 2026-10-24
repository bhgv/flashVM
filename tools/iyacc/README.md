# Inferno iyacc bootstrap

[Русский](README.ru.md) · [Manifest](UPSTREAM.json) · [mk support](../mk/README.md)

Source: bhgv/Inferno-OS-bhgv experiment, 3e3536578b969cf1a460210d8ec667713869ccfc.
17 selected upstream files include utils/iyacc/yacc.c, yaccpar, original mkfile,
three extra lib9/BIO translation units and notices/licenses. Upstream utils/iyacc
contains a C generator and skeleton; the Limbo grammar is limbo/limbo.y in the
selected compiler snapshot. No system yacc/bison or whole Inferno tree is needed.
The tool shares the explicit source-built mk support archive; it does not copy
another whole lib9/BIO library. Defaults can be overridden by MK_SOURCE_ROOT and
MK_SUPPORT_ROOT when building separately.

One invocation from the workspace root:

```sh
./build-workspace.sh --projects iyacc,limbo-bootstrap,ide
```

The script builds mk support, iyacc, regenerates y.tab.c/.h from actual limbo.y,
compiles the temporary native Limbo and then idecore.dis/.sbl. CMake 3.10 is enough.
The copied lexer is compiled beside the fresh token header; imported source/header
files are never overwritten. A standalone bootstrap CMake build may omit
IYACC_EXECUTABLE/IYACC_ROOT and use committed generated artifacts as before.

Pure native tools: `./build-workspace.sh --backend mk --projects iyacc`.
The original yaccpar is copied into BUILD/iyacc/utils/lib/yaccpar, matching iyacc's
ROOT lookup. The executable embeds that build root; moving it requires setting
ROOT to the relocated skeleton root. The binary is a development build product.

`--tests` exercises actual limbo.y generation and exact token-header compatibility.
The native compiler product test then compiles real Dis/debug outputs. The full
C/Limbo parser and runtime are still the temporary Inferno bootstrap: all new own
AS/Limbo/FIR parsers remain Coco/R. Linux AMD64 is tested; no other host acceptance
or upstream bug fix is claimed. Adapter scripts and mkfile are outside upstream/.

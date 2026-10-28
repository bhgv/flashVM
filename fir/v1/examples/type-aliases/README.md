# Executable Limbo type aliases

[Русский](README.ru.md) · [Contract](../../doc/SOURCE_ALIASES.md) · [Scalar example](main.b) · [Hybrid table](table.b)

Use matching FIR 0.49 tools and DisVM 0.32. Run from the repository root after building the toolchain. Every nonblank example line has an explanatory comment. main.b demonstrates canonical binding in the declaring scope: Count stays int even after the module declares Base as real.

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/type-aliases/main.b build/aliases.fir -g build/aliases.debug.json
build/fir/fir1 check build/aliases.fir
build/fir/fir1 opt build/aliases.fir build/aliases.opt.fir
build/fir/fir1 dis build/aliases.opt.fir build/aliases.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 2 build/aliases.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 2 build/aliases.dis
build/fir/fir1 object build/aliases.opt.fir build/aliases.swo
build/swf-linker/swl link build/aliases.swf build/aliases.swo
```

Both Dis runs produce DATA 0 42 and DATA 8 1. The second word records trace completion. The SWF displays its numeric result through trace when run by a host with trace output; opening a window alone is not a result check. This is the source → FIR → SWO → linker → SWF path. The Dis command uses the existing direct backend.

Repeat the first six commands with table.b as input to exercise big, string and hybrid-table aliases. Keep a distinct output basename. Its result is also 42. The table backend is Dis-only; do not send that program to the numeric SWF backend. The teaching value() wrapper is not the complete Inferno init ABI.

Compilation resolves aliases before FIR emission. It does not allocate alias objects or change runtime representations. Unknown/forward/self aliases, duplicate declarations, more than 128 aliases and value locals shadowing aliases are rejected. Module aliases precede the value export. General imports, arbitrary function bodies and alias-name casts remain outside this adapter; the interface compiler has a separate, broader alias resolver. Debug origins retain UTF-8 source byte offsets.

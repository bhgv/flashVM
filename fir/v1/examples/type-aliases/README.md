# Executable Limbo type aliases

[Русский](README.ru.md) · [Contract](../../doc/SOURCE_ALIASES.md) · [Scalar example](main.b) · [Hybrid table](table.b)

Use matching FIR 0.51 tools and DisVM 0.32.1. Run from the repository root after building the toolchain. Every nonblank example line has an explanatory comment. main.b demonstrates canonical binding in the declaring scope: Count stays int even after the module declares Base as real.

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

Compilation resolves aliases before FIR emission. It does not allocate alias objects or change runtime representations. Forward references in the complete file/module prefix are accepted. Unknown targets, self/mutual cycles (even unused), duplicates, more than 128 aliases and value locals shadowing aliases are rejected. Module aliases precede the value export. General imports and arbitrary function bodies remain outside this adapter; the interface compiler has a separate, broader alias resolver. Debug origins retain UTF-8 source byte offsets.

[casts.b](casts.b) demonstrates a forward result alias and module alias casts. Repeat the Dis commands with this source; both modes return 42. `I digits` parses signed64 and narrows to -1, then Result widens it before addition. A malformed decimal still raises even when its result is unused; optimization preserves that effect. Numeric int/real casts through aliases are identity-only in their homogeneous profiles. The big/table profile’s string/int/big casts reuse the existing typed conversion contract; a table object cannot be cloned by spelling a table alias as a cast.

The module/file shadow in main.b is a FlashVM lexical extension; native Inferno rejects that redeclaration. The common unshadowed scalar oracle is described in the contract.

## GC owner lifetime tutorial — runtime 0.32.1

[gc-owners.b](gc-owners.b) keeps a deleted string key in an independent snapshot while 64 loop iterations allocate formatted strings, narrow them to int, copy keys into a table and delete them again. The observable sum is 1952 and the retained key converts to 42. `countof`, rather than dense `len`, verifies that no dictionary entries remain. Every nonblank line is commented.

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/type-aliases/gc-owners.b build/gc-owners.fir -g build/gc-owners.json
build/fir/fir1 opt build/gc-owners.fir build/gc-owners.opt.fir
build/fir/fir1 dis build/gc-owners.opt.fir build/gc-owners.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 build/gc-owners.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 4 build/gc-owners.dis
```

Expected DATA words at offsets 0/8/16/24: **42, 0, 1, 0**. The last two are the selected/unselected return flags. Stats must report `tables_live=0 keys_live=0`; the acceptance test also checks repeated GC at quantum 1 and actual native dispatch counters. Run raw and optimized images, then repeat with quantum 2048. The installed SDK performs the same owner/result checks after relocation. This is a Dis-only table program; the scalar SWF path is covered by the earlier examples. These counters establish host table/key teardown, not a separate count of every native String allocation.

## Local type scopes — compiler 0.51

[local.b](local.b) declares Number and Result inside value(), shadows Number as int and then string in sibling blocks, and declares Step in a loop body. It returns 42. Compile it using the earlier Dis commands with a separate output basename. The expected words are DATA 0 42 and DATA 8 1. This hybrid expression program is Dis-only; numeric local-alias programs are also exercised through SWF by the test suite.

Local aliases resolve immediately against visible declarations. An unresolved local forward target is rejected; file/module forward binding remains supported. Leaving a block retires its aliases and restores the parent's type environment. Declarations emit no runtime value, frame slot or FIR instruction, so a loop-body typedef is compiled once. Type and value declarations cannot use the same name in one scope; value locals still cannot shadow aliases in this bounded profile. Type aliases in for init/step clauses are rejected; ordinary typed value declarations there retain their previous behavior.

The 128-alias limit counts every declaration in the translation unit, including retired siblings. Names do not leak into another compiler-library request after errors or successes. Prefix aliases are resolved before the signature; body-local names are unavailable in that signature.

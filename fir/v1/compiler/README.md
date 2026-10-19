# FIR compiler and C99 API

[Русский](README.ru.md). [Build](../README.md).

```sh
build/fir1/fir1 check INPUT.fir
build/fir1/fir1 print INPUT.fir OUTPUT.fir
build/fir1/fir1 opt INPUT.fir OUTPUT.fir
build/fir1/fir1 swf INPUT.fir OUTPUT.swf
build/fir1/fir1 actions INPUT.fir OUTPUT.actions
build/fir1/fir1_source as2 --input INPUT.as OUTPUT.fir
```

`check` parses/verifies without output; `print` canonicalises; `opt` also applies
peephole; `swf` writes FWS8; `actions` writes raw AVM1 records including ActionEnd.
`swf` does not implicitly optimise: choose the input explicitly. Source adapter
language is `as1`, `as2`, `as3` or `limbo`; `--input` precedes the input path;
last argument is the output path. See [coverage](../doc/COVERAGE.md).
No shell expansion or implicit include/module search is performed.
Status: 0 success, 1 input/semantic rejection, 2 CLI/I/O/backend failure in fir1;
fir1_source returns 1 for parse/translation/output failures and 2 for CLI/setup.

## Source map

- [fir1.atg](fir1.atg): FIR scanner/parser and semantic hooks.
- [src1.atg](src1.atg): tutorial source scanner/parser, precedence and lowering.
- [source.c](source.c): source adapter CLI; separate generated globals.
- [builder.c](../src/builder.c): bounded IR construction, version/UTF-8 checks.
- [ir.c](../src/ir.c): verifier and printer.
- [swf.c](swf.c): AVM1 generation, relocation and SWF container.
- [main.c](../src/main.c): FIR command driver and temporary output staging.

## Internal C API

Declarations: [fir.h](../include/fir.h). Returns 1 on success, 0 on failure.

| Function | Parameters and ownership |
| --- | --- |
| `fir_parse(input, module, error, capacity)` | Read binary FILE at current position; caller-owned output module; writable diagnostic and its byte capacity. Generated globals mean one parse per process, not a thread-safe embedding API. |
| `fir_verify(module, error, capacity)` | Check caller-owned parsed module; diagnostics as above. |
| `fir_print(output, module)` | Write canonical text to caller-owned FILE; does not close it. |
| `fir_optimise(module, error, capacity)` | Mutate verified module, verify again; preserve observable effects. |
| `fir_swf(output, module, raw, error, capacity)` | Verified module only; raw=0 FWS8, raw!=0 standalone actions; does not close stream. |

Diagnostics report FIR input lines; source adapter syntax diagnostics also include
columns/byte offsets. A full debugger/source map is not implemented. Modules
are relatively large: allocate on heap as the CLI does. The IR ABI is internal.

Backend values use reserved movie-local names `__fir1_value_N`; the trace-only
movie has a zero-size stage. This is not a renderer/frontend example. i32 add/sub
are wrapped by BitOr 0; multiplication uses 16-bit partial products to avoid
binary64 rounding loss. Branch offsets are resolved after emission and checked.
Channel methods require the existing FlashVM extension installed by the host.
Both generated parsers use the same explicitly configured external Coco/R;
their globals never enter the same executable. Do not edit generated build files.

Local CMake entry: configure this directory with the same COCOR_ROOT and
COCOR_EXECUTABLE flags; tool executables are under the build directory’s `v1/`.

# FIR 0.3.2 compiler and C API

The compiler produces `.swo` objects, never SWF containers. Linking belongs to
independent [SWL](../../../linker/README.md). Optimization runs separately or
through explicit `FirRunOptions.optimise`; CLI programs invoke the same C API.

```sh
build/fir1/fir1_source as2 --input a.as a.fir --module a
build/fir1/fir1 check a.fir
build/fir1/fir1 print a.fir canonical.fir
build/fir1/fir1_opt a.fir optimized.fir
build/fir1/fir1 object optimized.fir a.swo
build/linker/swl link movie.swf a.swo
```

[Embedding API](API.md), [SWO1 format](../../../linker/FORMAT.md),
[coverage](../doc/COVERAGE.md). `fir1 swf`, `fir1 actions` and `fir_swf` are removed;
there is no hidden linker bypass. `fir1_source` supports numeric tutorial profiles
of AS1/AS2/AS3/Limbo, not complete languages. AS2/AS3 annotations are Number;
AS3 currently lowers to AVM1, not ABC. Linked module names must be distinct.

Sources: [fir1.atg](fir1.atg), [src1.atg](src1.atg), [lower.c](lower.c),
[source_api.c](source_api.c), [pipeline.c](../src/pipeline.c),
[object.c](object.c), [verifier](../src/ir.c).
External bhgv/coco-r-C generates parsers in build. Wrappers reset errors and
convert legacy scanner fatal paths into returned failures. Namespaces allow
parsers in one executable, but calls to each parser must be serialized. IR
modules are large; allocate them on the heap.

Streams remain caller-owned; NULL/0 diagnostics are allowed. `fir_run` accepts a
task, streams and options; `fir_optimise` operates directly on a module;
`fir_object` verifies before SWO1 emission. API/structures remain internal;
a stable binary ABI is not promised. CLI: 0 success, 1 pipeline/output failure,
2 usage/initial opening failure. Validation failures preserve existing output;
write failures may leave a partial file.

[Working source syntax](../doc/SOURCE_SYNTAX.md).

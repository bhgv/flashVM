# SWF 1-8 loading and current AVM1 additions

The loader accepts FWS for versions 1-8 and zlib-compressed CWS for versions
6-8. ZWS and versions above 8 are unsupported. Decoding produces canonical FWS
bytes in context-owned storage before committing a successful load. Failure
preserves the old movie. Inspection accepts the same formats and commits its
output only on success.

The declared decoded length is checked before allocation. Inspection and
contexts with no configured limit use [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) (64 MiB).
A nonzero [FVM_Config](../include/flashvm/flashvm.h#L51).memory_limit_bytes bounds the decoded movie buffer; this
is **not yet a global VM allocation accounting mechanism**. Oversized movies
return OUT_OF_MEMORY. A CWS must contain exactly one complete zlib stream,
consume all compressed input and produce exactly FileLength-8 bytes. Truncated,
corrupt, excess-output and trailing-input streams are rejected. RECT must
have nonzero bit width. End must be empty and terminate the file; ShowFrame
must be empty. Tag payload bounds and extended lengths are checked.

[`fvm_swf_execute_actions`](../include/flashvm/avm1.h#L73) reads top-level DoAction tags from normalized FWS,
including movies loaded as CWS. It is not a timeline player: sprites, display
objects, events, shape decoding and unsupported tags are not implemented by
this function. Unknown tags can be structurally valid but unimplemented.
[`fvm_advance`](../include/flashvm/flashvm.h#L74) still returns UNSUPPORTED.

## AVM1 value transport

ActionPush now handles string, float, null, undefined, register, boolean,
double, signed 32-bit integer, Constant8 and Constant16. Float/double are
reconstructed mathematically from IEEE fields without unaligned casts or
host-endian assumptions. AVM1 double uses high little-endian word followed by
low little-endian word. NaN, infinities and subnormals are represented with
standard C++98 floating support; target runtime coverage is still required.

ActionConstantPool replaces the current pool and validates exact string
count/payload consumption. Named DefineFunction captures its defining pool.
ActionStoreRegister supports the existing four-register action context and
does not pop. Register values are traced, including across explicit GC.
DefineFunction2's register allocation and preload flags are not implemented.

Bytecode string literals and constant-pool strings are copied into context
storage before being pushed: a global property cannot retain a pointer into
freed/reused host bytecode or a temporary function/pool snapshot. These strings
remain allocated until context destruction; string GC remains outstanding.

Tests: `swf_compressed.c` (valid CWS, truncation, corrupt stream, length and
allocation limits, transactional reload), `avm1_push_values.cpp` (floating
values, both pool indices, captured pool, register GC, destroyed/reused literal
buffer and malformed payloads), plus existing SWF/AVM1 regressions.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `fvm_swf_execute_actions` | function | [include/flashvm/avm1.h:73](../include/flashvm/avm1.h#L73) |
<!-- source-links:end -->

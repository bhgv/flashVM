# FIR 1.0 format

[Русский](FORMAT.ru.md). This is the implemented subset, not a promise that all
operations from the earlier architecture discussion already exist.
Authoritative syntax: [fir1.atg](../compiler/fir1.atg).

## Version, encoding and comments

The physical first line is exactly `FIR 1.0` followed by LF or CRLF. No BOM,
blank line or comment precedes it. The file suffix is `.fir`; directory `v1`
denotes the major family, while header `1.0` identifies this exact grammar.
Unknown versions are rejected before parsing. A new accepted grammar requires
an explicit reader change; silently interpreting future minor versions is forbidden.
Implementation version 0.2.2 and FlashVM development version are independent.

UTF-8 is validated, including comments; embedded NUL and malformed UTF-8 fail.
`//` runs to newline/EOF. `/* ... */` is non-nesting and must terminate.
Quoted strings protect comment delimiters. Escapes: `\n`, `\r`, `\t`, `\"`, `\\`.
Strings have no physical newlines or NUL. Unicode escape syntax is not yet supported.
Comments do not affect semantics and are not preserved verbatim by print/opt;
canonical output includes original input-line notes instead. Full source maps
and original source-language debug variable records are future work.

## Module and control flow

```text
FIR 1.0
module @decision
func @main() -> void {
^entry:
    %a: i32 = const.i32 6
    %b: i32 = const.i32 7
    %test: bool = i32.lt.signed %a, %b
    if %test, ^yes, ^no // true target, false target
^yes:
    trace %a
    jump ^end
^no:
    trace %b
    jump ^end
^end:
    return
}
```

Exactly one zero-argument `@main() -> void` is implemented. The first written
block is entry. Each block has a unique label and ends in `if`, `jump` or
`return`; nothing may follow a terminator before the next label. All blocks
must be reachable. `if` requires bool and exactly two labels. Loops can jump
backward, but there are no block arguments, phi, mutable slots or function calls
in this version, so general induction-variable loops cannot yet be expressed.

Symbols: `%` values, `^` blocks, `@` module/function. Names start with an ASCII
letter or underscore; following bytes may also be digits or dots. Names are
case sensitive. A value has exactly one definition. Definitions must dominate
every use; a same-block definition must precede its use. Forward labels are allowed.

## Supported operations

| Operation | Inputs → result | Semantics |
| --- | --- | --- |
| `const.i32 N` | → i32 | Signed decimal integer, -2147483648..2147483647 |
| `const.f64 N` | → f64 | Finite decimal literal, optional fraction/exponent; binary64 |
| `const.bool true/false` | → bool | Boolean value |
| `const.string "text"` | → string | UTF-8 text, supported escapes |
| `copy %a` | T → T | Same logical value, including channel wrapper identity |
| `i32.add.wrap %a, %b` | i32,i32 → i32 | Addition modulo 2^32, signed interpretation |
| `i32.sub.wrap %a, %b` | i32,i32 → i32 | Subtraction modulo 2^32 |
| `i32.mul.wrap %a, %b` | i32,i32 → i32 | Exact low 32 product bits, including large operands |
| `i32.lt.signed %a, %b` | i32,i32 → bool | Signed comparison |
| `i32.eq %a, %b` | i32,i32 → bool | Equality |
| `f64.add/sub/mul %a, %b` | f64,f64 → f64 | Strict floating arithmetic; three separate opcodes |
| `as.add %a, %b` | scalar,scalar → any | SWF8 AVM1 Add2 coercion/concatenation |
| `as.to_number %a` | scalar → f64 | SWF8 AVM1 ToNumber |
| `trace %a` | scalar → void | ActionTrace string conversion and delivery to host sink |
| `if %c, ^yes, ^no` | bool → void | Exactly two explicit destinations |
| `jump ^block` | → void | Unconditional transfer |
| `return` | → void | Finish this top-level action block |

Each result is written `%name: type = operation`. `scalar` means i32/f64/bool/
string/any, excluding channel. `any` currently comes from dynamic addition or
channel receive; arbitrary object construction is absent. `f64` constants must
be finite, but arithmetic may produce infinities, NaN or negative zero. No
fast-math identities are assumed. Decimal parsing uses the executable's initial C locale.

## Existing FlashVM channel extension

Add `requires flashvm.channels` after `module` to opt into the extension.

| Operation | Inputs → result | Existing method |
| --- | --- | --- |
| `limbo.chan_create` | → channel | `channel.create()`; unbounded queue |
| `limbo.chan_send %ch, %value` | channel,scalar → void | `ch.send(value)`; discard successful true |
| `limbo.chan_receive %ch` | channel → any | `ch.receive()`; may block |
| `limbo.chan_close %ch` | channel → void | `ch.close()`; discard returned value |

Example: [channels/main.fir](../examples/channels/main.fir). The host installs
`fvm_extensions_install(context)` before execution. Operations use AVM1 CallMethod
and the existing runtime; they add no new VM opcode. Errors propagate through
AVM1 execution. Close is not destruction; wrapper references are VM-owned.
Queue capacity, typed receives, alt/select, spawn and graph-typed FIR values are
not implemented here. Existing channel support does not mean these language
lowerings are complete. See [extension API](../../../flashVM/doc/ACTIONSCRIPT_EXTENSIONS.md)
and [capacity semantics](../../../flashVM/doc/CHANNEL_CAPACITY.md).
Plain players without the extension cannot execute this profile.

## Effects and optimisation contract

Instruction order is execution order inside a block. Constants, copy and typed
arithmetic/comparisons are total and effect-free in this FIR model. Float NaN
payloads and host floating exception flags are not observable FIR state.
Dynamic conversion/addition, trace and channel operations are conservatively
preserved. Channel operations can allocate, mutate queues, fail or block.
No pass moves them across other effects. Exception handlers/invoke and explicit
effect tokens are not part of 1.0. Unknown operations always fail.

## Representation and bounds

[C99 structures](../include/fir.h) hold instructions, values and labels using
indices, not VM-private pointers. [Builder](../src/builder.c) bounds every append.
Initial limits: 2 MiB input; 4096 instructions/values; 256 blocks; 255-byte token
spelling (including string quotes/escapes); decoded strings fit that bound.
Backend buffers are each 2 MiB. AVM1 jumps use signed 16-bit offsets; out-of-range
jumps are rejected, not truncated. No automatic long-jump trampolines yet.

The generated parser owns global state. One input per process, no concurrent
parse calls. The IR structures are an internal C99 API, not a stable embedding
ABI or binary file format. The text is the interchange format. Failed syntax,
type, dominance or backend-range checks do not overwrite an existing output.
OS write failures can still leave a partial destination file.

The effect table lives in `optimiser/effects.c`: `FIR_READS_MEMORY`,
`FIR_WRITES_MEMORY`, `FIR_ALLOCATES`, `FIR_MAY_THROW`, `FIR_MAY_CALL_USER`,
`FIR_MAY_SUSPEND`. C API `fir_effects(op)` returns the mask; an unknown opcode
receives `FIR_ALL_EFFECTS`. Control instructions have no result and are not
removed by DCE even with a zero effect mask.

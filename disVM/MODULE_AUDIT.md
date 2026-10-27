# Inferno module review: useful code and actual boundaries

[Русский](MODULE_AUDIT.ru.md) · [Selected libraries](libraries/README.md) ·
[Runtime](runtime/README.md) · [Synchronization](SYNC.md)

This review follows the retained Dis interpreter, builtin registration, hosted
Sys export closure and selected ordinary Limbo libraries. It distinguishes code
already executed by FlashVM DisVM from code that still needs a host interface.
It is not a claim that every module in Inferno has been ported.

| Area | Useful existing code | FlashVM DisVM state / next concrete use |
| --- | --- | --- |
| Dis core, heap, stack, channels | RC/GC roots, frames, ALT cancellation, guarded native leaves | Retained and executed; keep typed NC64 adapters separate from the standard ABI. |
| `$Sys` | Formatting, UTF helpers and native I/O contracts | Hosted read/write/create/seek and standard descriptors work. [stat/fstat/dirread](runtime/FILESYSTEM.md) use OS-selected ports. Process and namespace/network exports still have unsupported paths. |
| `$Loader` | Instruction/type/link introspection | Builtin already registered and exercised. Useful for an IDE disassembly view; dynamic construction exports need broader acceptance. |
| `Dis` | Object decoder, opcode names and source-path metadata | Already source-built. Its object parser is classic-format code; do not feed NC64 images to it as if the formats matched. |
| `Debug`, `Exception`, `Profile` | Symbols, stack/control protocols and profiling models | Debug is packaged; `/prog`, process controls and profiling devices remain the main integration work. A declaration is not a running debugger. |
| `Bufio`, `String`, `Arg` | Rune-aware buffers, text/numeric parsing and argument handling | Already packaged and exercised. Reuse these helpers rather than duplicating text code in IDE services. |
| `Readdir` | Directory sorting/filtering | Actual enumeration and sorting are exercised through hosted Sys.dirread and OS ports. |
| `Regex` | Ordinary Limbo regexp parser and finite-state execution | Added to the selected source-built libraries in this series; actual rune/capture regression runs on DisVM. Useful for IDE search and later explicit regexp-slice lowering. |
| `Lock` | A channel-backed semaphore | Useful as a reference for service serialization. It is not an owner-aware reentrant object mutex and must not replace the existing table lease protocol blindly. |
| `Styx`, `Styxservers` | 9P2000 wire codec and server helpers | Packaged codec is useful for transport work. A codec alone is not a mounted namespace, cancellable transport or automatic remote object lifetime registry. |
| `$Srv`, `$Math` | Resolution, scalar/vector operations and format conversions | Builtins already registered. Resolution may block the single owner; native math uses reviewed host adaptations. |
| `$Draw`, `$Tk`, `$Prefab`, `$Freetype` | Inferno UI implementations | Separate from our FlashVM graphics frontend; importing these implementations is not required to render the IDE. |
| `$IPints`, `$Crypt`, `$Keyring` | Arbitrary integers and cryptographic services | Not added by this review. They need their own ABI/ownership/security acceptance rather than bulk import. |

## Confirmed useful transfer: Unicode Regex

The old Regex operator tags started at `1<<16`, colliding with valid Unicode
U+10000…U+1000B. An actual DisVM run of the unmodified library rejected the literal
U+10000 as an invalid expression. The corrected tags start at `1<<21`; the parser
sentinel derives from the tag base. Public compile/execute signatures stay intact.

The fix was published back to Inferno experiment as **320e6581e425c014f8a9ba313352b664925a709f**,
then imported byte-for-byte. The native and standalone library builds now include
Regex; runtime/editor packages ship its source, interface, bytecode and symbols.
The installed SDK regression recompiles both Regex and its client using only
relocated headers/sources and executes both dispatch modes at quanta 1 and 2048.

The 85-case matrix covers all twelve previous collisions, ASCII/BMP, emoji and
U+10FFFF through literals, escaping, classes, alternatives and repeated captures.
Offsets are rune indexes, not UTF-8 byte offsets. The old source/interface pair
fails an active compile-result check; the corrected pair passes all four runs.
[Upstream regression](upstream/appl/lib/tests/README.regex-unicode.md).

This is the existing Limbo library running on DisVM. It does not enable regexp
search in the editor UI, add regexp-slice syntax to our Coco/R frontend or create
a dedicated native regexp JIT. Full Regex source currently needs the retained
ordinary-Limbo bootstrap; our Coco/R big/table compiler remains a smaller profile.

## Priority after this review

First connect concrete Sys metadata/directory operations and `/prog`-style debug
services to the existing portable host boundaries. That enables real directory
browsing, classic Dis metadata readers and debugger views without importing a
second graphics stack. Keep module-format decoding explicit: NC64 and classic
Dis need different accepted layouts.

For text search, use the now-tested Regex API behind a typed IDE request/reply
service with bounded work and authored occurrence diagnostics. The service must
state how cancellation and owner scheduling behave; a long pure-Limbo match can
be preempted by VM quanta, but a future native blocking operation must not freeze
the broker. Regex replacement/backreference syntax and transparent `any`/proxy
operators still require separate language/FIR contracts.

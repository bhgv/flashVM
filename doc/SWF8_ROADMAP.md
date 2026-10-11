ActionScript text checkpoint: named native TextFormat construction, extent measurement, a host-created uniform-format TextField bridge, UTF-16 range editing and format reads are now tested. See STANDARD_TEXT.md. MovieClip creation, SWF fonts, HTML, styled range writes and complete display/input behavior remain outstanding.

Native text checkpoint: owned single-font horizontal layout, wrapping, bidi/script runs, measurement and editable block/glyph paths are tested. See [TEXT_LAYOUT.md](TEXT_LAYOUT.md). SWF display-list and embedded-font integration remains outstanding.

Numeric library checkpoint: opt-in Math methods/constants and primitive
Number/Boolean/isNaN/isFinite are now tested; see STANDARD_NUMERIC.md.
Boxed classes, first-class functions, property attributes and complete coercion
remain outstanding. Other scope below is unchanged.

# SWF 8 compatibility contract and remaining implementation

The project is **not a complete Flash Player or SWF 8 VM**. A passing subset
suite is not evidence of full compatibility. This document tracks the scope
requested on 2026-10-11, not an implemented feature list.

| Area | Tested now | Remaining acceptance scope |
| --- | --- | --- |
| Containers | Bounded FWS/CWS parsing and normalization | Every SWF<=8 tag decoder, nested sprite validation, fuzz/corpus coverage |
| AVM1 | Named functions, parameters, objects, basic arithmetic/calls/branches, all Push encodings, pools, four registers | Full opcode semantics, coercion/version differences, Function2, closures, with/try/throw, enumeration, delete, Unicode/string semantics |
| Timeline | Top-level DoAction execution in file order | Frame scheduling, play/stop/goto, DoInitAction, nested MovieClip timelines, action/event ordering |
| Objects | Generic objects/prototypes, host properties/callbacks | Object/Function/Array/String/Number/Boolean/Math/Date/Error and all versioned AS1/AS2 methods/properties |
| Player system APIs | Partial host input/network helpers | Stage, Key, Mouse, Selection, System, capabilities/security, broadcaster, XML/XMLSocket, LoadVars, SharedObject, timers, loader semantics |
| Display and text | RGBA8 antialiased polygon adapter | Display list, depths/transforms/masks, shapes/morphs/bitmaps, gradients, text/fonts, drawing APIs, filters/blends, hit testing |
| Media | No SWF decoder/player | Sound codecs/streaming/mixing, video/codecs/synchronization, BitmapData and SWF8 media APIs |
| GC | Tested cycles/finalizers/resurrection/native/AVM1 roots | Collectable strings, allocation accounting, coverage of all new object types and JIT safepoints |
| Extensions | GC/thread/channel namespaces; Lua-inspired C stack callbacks | C modules/upvalues/registry/errors, further requested network operators/protocols, full callable dispatch |
| JIT | No native machine-code backend | IR, deoptimization/fallback, W^X and cache-flush ports, every ISA backend and differential tests |
| Platforms | Linux strict build, sanitizer suite, CMake/CTest | Actual Windows/macOS/Android builds and execution; each ABI/CPU target |

## JIT targets

Interpret `ruscv` as the requested 32-bit RISC-V target (RV32); `riscv64` means
RV64. Required backend families: ARM32, AArch64, x86, x86-64, RV32 and RV64.
Each needs documented instruction-set baseline and ABI; compilation must not
assume host CPU equals target CPU. Floating point and software fallback,
unaligned access, pointer size, calling conventions, branch relocations,
executable allocation/protection and instruction-cache synchronization require
explicit design and tests. OS operations belong in `ports/`.

JIT is not implemented. Requesting FVM_ENABLE_JIT now fails configuration
instead of silently producing an interpreter-only build. Required proof:
interpreter/JIT differential execution of real AVM1, unsupported-op fallback,
GC during compiled execution, exceptions/cancellation, branch/link bounds and
native callback ABI, all on each actual target or a documented emulator.

## Next implementation order

1. Finish AVM1 semantic/value model and root ownership, add compiler-generated
   SWF corpus fixtures and malformed-input fuzzing.
2. Implement standard object library and versioned SWF<=8 behavior.
3. Implement MovieClip timeline/events/display list, then SWF shape and bitmap
   decoders over the tested raster adapter; text/masks/filters follow.
4. Complete system services, media and requested extensions with platform
   adapters and source-vendored dependencies.
5. Add JIT IR and one reference backend, then all remaining CPU/ABI backends;
   keep interpreter differential validation as a release gate.
6. Validate Windows/Linux/macOS/Android platform/architecture matrix and real
   movies. Publish explicit supported behavior and failures in both languages.

New code uses C89/C++98 without STL containers. Source dependencies added in
this phase live in `external/`; historical mini_regex remains in `external/`.
Preserve upstream source notices; maintain readable formatting/comments.

## Repository policy

The source ZIP with its complete `.git` is authoritative. The remote
https://bhgv.pythonanywhere.com/git/FlashVM/ is a secondary mirror. Push/update
it **only on an explicit user command**, never automatically during builds,
commits, packaging or future development. This phase does not update it.

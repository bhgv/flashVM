# FlashVM

Portable SWF 1-8 / AVM1 runtime project (early scaffold).

## Compatibility policy

- Public headers: C89-compatible C ABI; no STL or C++ types.
- Core: C++98; no C++11+ features, no OS-specific dependencies in core.
- Optional backends: rendering, media, JIT, platform windowing.
- Native extension interfaces will be versioned and GC-safe.
- No claim of SWF playback or ActionScript execution yet.

## Build

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

For older CMake versions without `-S` / `-B`, use `mkdir build && cd build && cmake ..`.

## Current status

Only runtime/context creation and ABI/configuration smoke tests are implemented.
SWF parsing, AVM1, JIT, rendering, media, extensions and player are planned.
Unimplemented entry points return `FVM_UNSUPPORTED`.

## Current implementation status

The portable C89-facing API supports runtime/context lifecycle, basic tagged
values, context-owned objects, object properties, globals and registered native
callbacks. `include/flashvm/value.h` declares these operations.

This is an early prototype, **not yet a SWF interpreter**. No AVM1 bytecode,
SWF parsing, JIT, renderer or media support is implemented. Object and string
storage currently lasts until context destruction; there is no garbage
collector. API object/string references must not outlive their context, and
contexts are not thread-safe. `fvm_load_memory` and `fvm_advance` return
`FVM_UNSUPPORTED`. Avoid untrusted native extensions; no sandbox is provided.

Build and test:

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

## Embedding and extending (implemented API)

- [Embedding API reference](doc/EMBEDDING_API.md)
- [Extending API reference](doc/EXTENDING_API.md)
- [Embedding example (C89)](examples/embedding/README.md)
- [Native extension example (C89)](examples/extending/README.md)

The examples are compiled and run by CTest. They demonstrate the **current**
object/value/native callback API, not ActionScript execution or dynamic plugins.

## Experimental AVM1 execution (stage 5)

The public C89 header `flashvm/avm1.h` exposes `fvm_avm1_execute()`
for an AVM1 action stream and `fvm_swf_execute_actions()` for top-level
`DoAction` tags in an already loaded **uncompressed FWS** file.
Register native callbacks with `fvm_register_function()` before executing.
The implementation supports `End`, `Push` (string, integer, boolean, null,
undefined), `Pop`, `GetVariable`, `SetVariable`, `Add`, `Subtract`,
`Multiply`, `Divide`, `Jump`, `If`, and `CallFunction` to registered native
functions. Unsupported instructions return `FVM_UNSUPPORTED`; malformed
input or stack operations return errors. There is no JIT, AS-defined
functions, display list, or general Flash Player compatibility yet.

Use `ctest --test-dir build --output-on-failure` to run the C89 integration
examples, the AVM1 interpreter test, and the SWF `DoAction` test.

## Stage 6: object operations, input mapper, optional SQLite

AVM1 now supports limited `NewObject` (`Object()` with zero arguments),
`GetMember`, and `SetMember`. Prototypes, arrays, AS-defined functions,
and movieclips are **not** implemented.

The host-configurable C89 gesture mapper converts already-recognized
multitaps/swipes into mouse-button identifiers, including wheel up/down.
It does not yet recognize touches or dispatch mouse events to Flash.

The optional SQLite adapter provides an in-memory/on-disk database C API.
SQLite is **not automatically exposed to ActionScript** and must be gated
through explicit host callbacks. See [input API](doc/INPUT_API.md) and
[SQLite API](doc/SQLITE_API.md).

Categorized examples with commented C89 code and AS1/AS2 sketches:
[embedding](examples/embedding/) and [extending](examples/extending/).
Examples labelled **design only** require backends not yet implemented.

## Prototype API (stage 7)

`doc/PROTOTYPES.md` describes the implemented prototype-chain operations.
`examples/embedding/prototypes/` includes a C89 example and a separate
ActionScript 2.0 reference example. Arbitrary ActionScript constructors and
functions are not yet supported.

### Stage 8: additional AVM1 stack instructions

The interpreter supports `PushDuplicate`, `StackSwap`, `Not`, and a limited
numeric-only implementation of `Equals`, `Less`, `And`, and `Or`. See
[`doc/AVM1_STACK.md`](doc/AVM1_STACK.md) for exact limitations. The
`flashvm_avm1_stack` test exercises the instructions using the public C API.

## Experimental ActionScript function execution

The AVM1 interpreter now supports a limited named, zero-argument
`DefineFunction` / `CallFunction` / `Return` path. See
`doc/AVM1_FUNCTIONS.md` and `examples/embedding/script_functions/`.
The feature is not yet compatible with general ActionScript functions.

## Русский язык / Russian documentation

Основная документация на русском: [README.ru.md](README.ru.md). Документация жестов и событий ввода: [INPUT_EVENTS.md](doc/INPUT_EVENTS.md). Пример на C89 и проектный пример AS2: [gesture_dispatch](examples/embedding/gesture_dispatch/README.md).

### Function parameters / Параметры функций (stage 11)

See `doc/AVM1_CALL_FRAMES.md` and `examples/embedding/function_parameters/`.
Named AVM1 functions now accept arguments and local variable declarations.
This is a partial implementation, not complete AVM1 function semantics.

Описание на русском: `doc/AVM1_CALL_FRAMES.md`. Поддерживаются параметры
именованных функций и локальные переменные; полная модель функций AVM1
ещё не реализована.

## Portable render jobs / Переносимые потоки отрисовки

See [English and Russian render threading documentation](doc/RENDER_THREADS.md)
and [the C89 example](examples/embedding/parallel_render/README.md).
The native thread adapters are in `ports/windows`, `ports/linux`, `ports/mac`,
and `ports/android`. **The current API executes host drawing jobs; full
MovieClip rasterization is not yet implemented.**

См. [двуязычное описание](doc/RENDER_THREADS.md) и
[пример C89](examples/embedding/parallel_render/README.md).
Переходники ОС расположены в `ports/`. Полноценная параллельная отрисовка
MovieClip ещё не реализована.

## Native input and network OS ports

See [OS ports API](doc/OS_PORTS.md) and [Russian translation](doc/OS_PORTS.ru.md). Native adapters live in `ports/{windows,linux,mac,android}`. C89 embedding example: `examples/embedding/os_ports`. Socket calls are blocking and not exposed to AVM1 yet; raw input is not automatically routed to Flash mouse handlers.

## Stage 14: touch recognition

`flashvm/gesture_recognizer.h` implements host-driven tap/multitap/swipe
recognition, with configurable thresholds and existing mouse-button mappings.
See `doc/GESTURE_RECOGNIZER.md` and
`examples/embedding/gesture_recognizer/`. Events currently reach C callbacks,
not AVM1 object handlers.

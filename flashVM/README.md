Development version: **0.693** (see [VERSION.txt](VERSION.txt)). New commit subjects
start with `v0.691 ` and an English description. Increase the subversion before each
commit and append `-v<version>` before `.zip` in delivered source/Git archive names.
This version identifies a development
snapshot; complete SWF 8 compatibility is still pending. CMake reads the same version.
Optional `--jit-emulator` platform validation records fresh Unicorn instruction/ABI evidence;
see [validation gates](doc/PLATFORM_VALIDATION.md).

v0.640: [Canonical sound frames](doc/SOUND_SNAPSHOT.md#canonical-frame-binding-v0640) / [Канонические звуковые кадры](doc/SOUND_SNAPSHOT.ru.md#канонический-кадр-v0640).

v0.639: [Sound metadata work](doc/SOUND_SNAPSHOT.md#metadata-work-v0639) / [Бюджет звуковых metadata](doc/SOUND_SNAPSHOT.ru.md#бюджет-метаданных-v0639).

v0.638: [Root sound work](doc/SOUND_SNAPSHOT.md#delivery-work-v0638) / [Бюджет доставки звука](doc/SOUND_SNAPSHOT.ru.md#бюджет-доставки-v0638).

v0.637: [Sound ordinal snapshots](doc/SOUND_SNAPSHOT.md) / [Снимки звуковых событий](doc/SOUND_SNAPSHOT.ru.md).

v0.636: [Scene graph safety](doc/SCENE_CONTROLS.md#graph-validation-and-legacy-depths-v0636) / [Граф сцены и старые глубины](doc/SCENE_CONTROLS.ru.md#граф-сцены-и-старые-глубины-v0636).

v0.635: [Scene traversal work](doc/SCENE_CONTROLS.md#scene-work-v0635) / [Бюджет сцены](doc/SCENE_CONTROLS.ru.md#бюджет-сцены-v0635).

v0.634: [Scene control snapshots](doc/SCENE_CONTROLS.md) / [Снимки управляющих записей](doc/SCENE_CONTROLS.ru.md).

v0.633: [Platform source evidence](doc/PLATFORM_VALIDATION.md#source-stability-v0633) / [Исходники при приёмке](doc/PLATFORM_VALIDATION.ru.md#стабильность-исходников-v0633).

v0.632: [Initializer progress](doc/INITIALIZER_PROGRESS.md) / [Бюджет инициализации](doc/INITIALIZER_PROGRESS.ru.md).

v0.631: [Frame action snapshots](doc/FRAME_BLOCKS.md) / [Блоки действий кадра](doc/FRAME_BLOCKS.ru.md).

v0.630: [Raster depth snapshots](doc/RASTER_DEPTH_ORDER.md) / [Отрисовка по глубинам](doc/RASTER_DEPTH_ORDER.ru.md).

v0.629: [Timeline depth order](doc/TIMELINE_DEPTH_ORDER.md) / [Порядок tick-обхода](doc/TIMELINE_DEPTH_ORDER.ru.md).

v0.628: [ClipEventKeyPress](doc/CLIP_KEYPRESS.md) / [Коды и доставка KeyPress](doc/CLIP_KEYPRESS.ru.md).

v0.627: [MovieClip input methods](doc/CLIP_INPUT_METHODS.md) / [Методы ввода MovieClip](doc/CLIP_INPUT_METHODS.ru.md).

v0.626: [Clip input broadcasts](doc/CLIP_INPUT.md) / [Ввод для дерева клипов](doc/CLIP_INPUT.ru.md), [C89](examples/embedding/clip_actions/README.md).

v0.625: [Retained ClipActions](doc/CLIP_ACTIONS.md) / [Обработчики ClipActions](doc/CLIP_ACTIONS.ru.md), [C89](examples/embedding/clip_actions/README.md).

v0.624: [MovieClip ticks](doc/MOVIECLIP_TICKS.md) / [Общий бюджет ticks](doc/MOVIECLIP_TICKS.ru.md), [C89](examples/embedding/movieclip/README.md).

v0.623: [updateAfterEvent and redraw requests](doc/UPDATE_AFTER_EVENT.md) / [Внеочередная отрисовка](doc/UPDATE_AFTER_EVENT.ru.md), [C89](examples/embedding/timers/README.md).

v0.622: [Timer wakeups](doc/TIMER_WAKEUP.md) / [Пробуждение таймеров](doc/TIMER_WAKEUP.ru.md), [C89](examples/embedding/timers/README.md).

v0.621: [Scheduled callbacks](doc/TIMERS.md) / [Таймеры](doc/TIMERS.ru.md), [C89](examples/embedding/timers/README.md).

v0.620: [getTimer global](doc/GET_TIMER.md) / [Глобальная getTimer](doc/GET_TIMER.ru.md), [C89](examples/embedding/get_timer/README.md).

v0.619: [AS1/AS2 URL globals and trusted global-call roots](doc/URL_FUNCTIONS.md) / [URL-функции и корни глобальных вызовов](doc/URL_FUNCTIONS.ru.md), [C89](examples/embedding/url_encoding/README.md).

v0.618: [Shared input event budgets and worker cancellation](doc/INPUT_EVENT_BUDGET.md) / [общий бюджет событий и остановка worker](doc/INPUT_EVENT_BUDGET.ru.md).

v0.617: [Bounded broadcaster registration](doc/AS_BROADCASTER.md#registration-work-limits-v0617) / [бюджет регистрации слушателей](doc/AS_BROADCASTER.ru.md#ограничение-работы-регистрации-v0617).

v0.616: [Broadcaster transports and player integration](doc/AS_BROADCASTER.md#transport-and-player-v0616) / [каналы и плеер](doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616), [two-context C89 tutorial](examples/embedding/broadcaster/README.md).

v0.615: [AsBroadcaster: generic application events](doc/AS_BROADCASTER.md) / [пользовательские события](doc/AS_BROADCASTER.ru.md), [C89/AS tutorial](examples/embedding/broadcaster/README.md).

v0.581: PCM queue/retry API, consumed-frame clock and optional real SDL2 output.
[EN guide](doc/SWF_SOUND_OUTPUT.md) / [RU](doc/SWF_SOUND_OUTPUT.ru.md),
[annotated C89 example](examples/multimedia/swf_audio_queue/README.md).
Software FIFO clock, not DAC position; automatic desktop A/V scheduling remains open.

v0.582 test player builds: [Linux x64, Windows x64, Android ARM64](doc/PLAYER_TEST_BUILDS.md)
and [three runnable SWF demos](examples/player/animation_audio/README.md).

v0.583: independent native audio worker and [prepared StopSounds](doc/AUDIO_STOP_BINDINGS.md); Android crackling remains under investigation after the device report.

v0.584: [Android PCM/WAV diagnostics / диагностика звука](doc/PLAYER_AUDIO_DIAGNOSTICS.md).

v0.586: [negotiated SDL audio block and reserve](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0586-publish-the-negotiated-device-quantum).

v0.587: [optional stop release / затухание остановки](doc/PLAYER_AUDIO_RELEASE.md).

v0.588: [ADPCM test-tone block restart fix / исправление тестового ADPCM-тона](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0588-adpcm-fixture-block-restarts).

v0.589: [Key polling and host keyboard snapshots / клавиатурный ввод](doc/KEYBOARD.md) ([RU](doc/KEYBOARD.ru.md)).

v0.610: [Native cursor application in SDL2, GTK3 and Qt5 / курсор во фронтендах](examples/frontends/README.md).

v0.609: [Mouse.hide/show and SWF5–8 cursor requests / видимость курсора из скрипта](doc/MOUSE.md#script-hide-and-show).

v0.608: [Context-owned cursor visibility / запрос видимости курсора](doc/MOUSE.md#cursor-visibility-request).

v0.607: [Native wheel normalization in SDL2, GTK3, Qt5 and evdev / колесо во фронтендах](examples/frontends/README.md).

v0.606: [Wheel events in the mixed FIFO / колесо в общей очереди](doc/INPUT_QUEUE.md), SWF6–8 interpreter/JIT checks.

v0.605: [Wheel listener delivery / доставка onMouseWheel](doc/MOUSE.md#wheel-events), [C89 tutorial](examples/embedding/mouse_wheel/README.md).

v0.604: [GTK3, Qt5 and Linux framebuffer/input/V4L2 frontends / новые фронтенды](examples/frontends/README.md).

v0.603: [Mixed gesture/Mouse/Key FIFO and bounded cancellation / общая очередь ввода](doc/INPUT_QUEUE.md), [C89 tutorial](examples/embedding/mixed_input/README.md).

v0.602: [Separate patch for the Clutter3D library / отдельный патч библиотеки](examples/frontends/clutter3d/LIBRARY_PATCH.md).

v0.601: [Clutter fork subpixel inverse and Linux EGL/GLES2 SDK / дробные координаты и EGL/GLES2](external/clutter3d/README.md#linux-eglgles2-profile-v0601).

v0.600: [Frontend tutorials / примеры фронтендов](examples/frontends/README.md): SDL2, selected Clutter/Cogl fork, relocated headless host; [SDK source and fixes](external/clutter3d/README.md).

v0.599: [Runnable host without SDL / запускаемый хост без SDL](examples/frontends/headless/README.md); shared pointer projection in the SDL FIFO.

v0.598: [Independent frame/event host boundary / независимый хост кадров и событий](doc/HOST_BACKENDS.md) ([RU](doc/HOST_BACKENDS.ru.md)).

v0.597: [Muted sound-bearing movie playback / запуск звуковых роликов с --mute](doc/PLAYER_POINTER.md#muted-movie-playback) ([RU](doc/PLAYER_POINTER.ru.md#ролики-без-звука)).

v0.596: [Player primary touch and shared input FIFO / касания и общая очередь ввода](doc/PLAYER_POINTER.md) ([RU](doc/PLAYER_POINTER.ru.md)).

v0.595: [Primary-pointer Mouse listeners / события касания и указателя](doc/MOUSE.md) ([RU](doc/MOUSE.ru.md)).

v0.594: [Selection observers / наблюдатели смены фокуса](doc/SELECTION.md#selection-listeners) ([RU](doc/SELECTION.ru.md#слушатели-selection)).

v0.593: [MovieClip focus notifications / уведомления смены фокуса](doc/SELECTION.md#focus-notifications) ([RU](doc/SELECTION.ru.md#уведомления-фокуса)).

v0.592: [Selection MovieClip subset / скриптовый выбор клипа](doc/SELECTION.md) ([RU](doc/SELECTION.ru.md)).

v0.591: [Explicit host keyboard focus / выбор клипа для клавиатуры](doc/KEYBOARD_FOCUS.md) ([RU](doc/KEYBOARD_FOCUS.ru.md)).

v0.590: [Key listeners and ordered transitions / слушатели и переходы](doc/KEYBOARD_LISTENERS.md) ([RU](doc/KEYBOARD_LISTENERS.ru.md)).

## Browse documentation

Install and launch the local documentation browser from the project root:

```sh
python -m pip install -r doku_viewer/requirements.txt
python view_docs.py
```

[Viewer guide](doku_viewer/README.md). Python 2.7.9 / Python 3.


Platform validation: reusable native AGG/Cairo/minimal runs and Android CTest/ADB
execution are described in [English](doc/PLATFORM_VALIDATION.md) /
[Русский](doc/PLATFORM_VALIDATION.ru.md). Non-Linux runtime acceptance remains open.

Native reference operations: JIT now supports variable writes/declarations,
member access and deletion; [contracts and checks](doc/JIT_REFERENCES.md).
Float/double literals and seven unary operations work in explicit/automatic JIT;
see [numeric contracts and checks](doc/JIT_NUMBERS.md).
Register reads/stores share prepared Function2, global and rooted root-block banks;
see [register contracts and checks](doc/JIT_REGISTERS.md).
ConstantPool and indexed Push preserve captured pools and runtime branch selection;
see [pool contracts and checks](doc/JIT_POOLS.md).
CallMethod/NewMethod now use shared rooted native dispatch;
see [method contracts and checks](doc/JIT_METHODS.md).
CallFunction/NewObject preserve lexical and versioned named lookup;
see [named-call contracts and checks](doc/JIT_NAMED_CALLS.md).
SetTarget/SetTarget2 select native timeline scope with versioned restoration;
see [target contracts and checks](doc/JIT_TARGETS.md).
Twelve string Actions now use shared counted/versioned native services;
see [string contracts and checks](doc/JIT_STRINGS.md).
InitArray/InitObject now preserve versioned literal count and entry semantics;
see [initializer contracts and checks](doc/JIT_LITERALS.md).
Legacy Equals/Less and And/Or now share native versioned coercion and hooks;
see [legacy contracts and checks](doc/JIT_LEGACY.md).
Extends/Implements/InstanceOf/Cast now use shared rooted class services;
see [class contracts and checks](doc/JIT_CLASSES.md).
GetProperty/SetProperty/TargetPath now use shared rooted native scene services;
see [display contracts and checks](doc/JIT_DISPLAY.md).
Native Trace/Throw/GetTime/RandomNumber: [contracts and checks](doc/JIT_EFFECTS.md).
Enumerate/Enumerate2: [contracts and checks](doc/JIT_ENUMERATE.md).
Native timeline commands: [contracts and checks](doc/JIT_TIMELINE.md).
CloneSprite/RemoveSprite/StartDrag/EndDrag: [contracts and checks](doc/JIT_SCENE.md).
WaitForFrame/WaitForFrame2: [contracts and checks](doc/JIT_WAIT_FRAMES.md).
GetURL/GetURL2/Call and stored-frame JIT: [contracts and checks](doc/JIT_DISPATCH.md).

The current primary repository is [GitHub](https://github.com/bhgv/flashVM).
Development commits are prepared on local work branches for downloadable format-patch
files. The agent does not commit on master or push. The user applies each patch
with git am --3way and publishes master from Termux. Author and committer must
both be `bhgv <bhgv@bhgv.bhgv>`. Each delivered patch includes the exact commands.

Версия разработки: **0.629** ([VERSION.txt](VERSION.txt)). Сообщения новых коммитов
начинаются с `v0.629 ` и описания на английском. Перед каждым коммитом повышается
субверсия; имена архивов исходников/Git получают постфикс `-v<версия>` перед `.zip`.
Полная совместимость с SWF 8 ещё
не завершена; номер обозначает текущий этап разработки. CMake читает эту же версию.

Сейчас основным является GitHub по полному адресу, указанному выше.
Автор и коммитер: `bhgv <bhgv@bhgv.bhgv>`. Историю принудительно не переписывать.

String checkpoint: native wrapper, UTF-16 indexed length/search/ranges,
fromCharCode, GC and graph transfer. See [English](doc/STANDARD_STRING.md) /
[Русский](doc/STANDARD_STRING.ru.md) for the tested subset and representation limits.

Global conversion checkpoint: parseInt/parseFloat prefix parsing and String(),
with C/AVM1 calls and allocation-failure tests. See [English](doc/GLOBAL_PARSING.md) /
[Русский](doc/GLOBAL_PARSING.ru.md).

Number checkpoint: boxed constructor, constants, valueOf/toString, integer radix
formatting and graph transfer. See [English](doc/STANDARD_NUMBER.md) /
[Русский](doc/STANDARD_NUMBER.ru.md).

Error checkpoint: constructor/prototype, native toString conversion, AVM1 instanceof,
GC roots and independent-process file transfer. See [English](doc/STANDARD_ERROR.md) /
[Русский](doc/STANDARD_ERROR.ru.md). Validation logs are delivered separately.

ActionScript text bridge: [English](doc/STANDARD_TEXT.md) / [Русский](doc/STANDARD_TEXT.ru.md).

Boolean checkpoint: boxed constructor, valueOf/toString, GC-traced native
prototype methods and independently installed worker namespaces. See
[English](doc/STANDARD_BOOLEAN.md) / [Русский](doc/STANDARD_BOOLEAN.ru.md).

Text editing checkpoint: UTF-16-indexed replaceText and uniform character/range
getTextFormat reads, with transactional validation and refreshed layout.
See [English evidence](doc/TEXT_EDITING_CHECKPOINT.md) /
[проверки на русском](doc/TEXT_EDITING_CHECKPOINT.ru.md).

Text layout checkpoint: [English](doc/TEXT_LAYOUT.md) / [Русский](doc/TEXT_LAYOUT.ru.md).

Font/vector checkpoint: bundled static FreeType 2.14.3, HarfBuzz 1.8.8,
libunibreak 6.1 and FriBidi 1.0.16; C89 native font/shaping APIs and editable
Bezier paths with AGG/Cairo fill/stroke/image masks. FONTS/NO_FONTS in DEFS.txt.
See [English](doc/FONT_LIBRARIES.md), [Русский](doc/FONT_LIBRARIES.ru.md) and
[validation](doc/FONT_LIBRARIES_CHECKPOINT.md). Full text layout/ActionScript
integration remains unfinished; the mirror is not updated automatically.

Dependency review checkpoint: `extern/` is merged into `external/`. Cairo 1.16.0,
Pixman 0.38.4 and SQLite 3.50.4 sources are bundled and preconfigured for local
builds. Edit top-level [DEFS.txt](DEFS.txt) to choose competing variants; default
is AGG + SQLite + HEADLESS. See [build keys](doc/BUILD_DEFINITIONS.md) and
[validation](doc/DEPENDENCY_CHECKPOINT.md). Screen adapters remain reserved keys.

Numeric standard library checkpoint: opt-in Math and primitive conversions
are now available. See [numeric API](doc/STANDARD_NUMERIC.md) for installation,
verified behavior and remaining compatibility limits. Phase 3: 95 Linux tests
and examples passed; mirror updates remain manual.

SWF 8 development checkpoint: bounded FWS/CWS loading, all AVM1 Push encodings,
constant pools/four registers, Lua-inspired C callbacks and bundled zlib/AGG.
This is still a partial VM. An explicit experimental Linux x86-64 JIT subset is
available; there is no complete player. See [native compiler](doc/JIT.md) and
[compatibility roadmap](doc/SWF8_ROADMAP.md), [loading](doc/SWF_LOADING.md),
[extension API](doc/LUA_STYLE_API.md), [builds](doc/BUILD_PLATFORMS.md).

# FlashVM

Portable SWF 1-8 / AVM1 runtime project (early scaffold).

## Compatibility policy

- Public headers: C89-compatible C ABI; no STL or C++ types.
- Core: C++98; no C++11+ features, no OS-specific dependencies in core.
- Optional backends: rendering, media, JIT, platform windowing.
- Native extension interfaces will be versioned and GC-safe.
- No claim of complete SWF playback or full ActionScript compatibility.

## Build

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

For older CMake versions without `-S` / `-B`, use `mkdir build && cd build && cmake ..`.

## Current status

FlashVM provides a partial AVM1 interpreter and an explicit experimental Linux
x86-64 native compiler subset. Build/run the interpreter/native example using
[these instructions](doc/JIT.md). Full player compatibility, other JIT backends
and roadmap acceptance remain open. Unsupported operations return errors.

## Historical initial prototype status

The portable C89-facing API supports runtime/context lifecycle, basic tagged
values, context-owned objects, object properties, globals and registered native
callbacks. `include/flashvm/value.h` declares these operations.

This is an early prototype, **not yet a SWF interpreter**. No AVM1 bytecode,
SWF parsing, JIT, renderer or media support is implemented. Object and string
storage currently lasts until context destruction; there is no garbage
collector. API object/string references must not outlive their context, and
contexts are not thread-safe. [`fvm_load_memory`](include/flashvm/flashvm.h#L72) and [`fvm_advance`](include/flashvm/flashvm.h#L74) return
[`FVM_UNSUPPORTED`](include/flashvm/flashvm.h#L46). Avoid untrusted native extensions; no sandbox is provided.

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

## Historical AVM1 execution checkpoint (stage 5)

The public C89 header `flashvm/avm1.h` exposes `fvm_avm1_execute()`
for an AVM1 action stream and `fvm_swf_execute_actions()` for top-level
`DoAction` tags in an already loaded **uncompressed FWS** file.
Register native callbacks with `fvm_register_function()` before executing.
The implementation supports `End`, `Push` (string, integer, boolean, null,
undefined), `Pop`, `GetVariable`, `SetVariable`, `Add`, `Subtract`,
`Multiply`, `Divide`, `Jump`, `If`, and `CallFunction` to registered native
functions. Unsupported instructions return [`FVM_UNSUPPORTED`](include/flashvm/flashvm.h#L46); malformed
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
SWF display-list MovieClip rasterization is not yet implemented; v0.13 adds
solid-vector rendering of host-authored MovieClip models.**

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

### Deferred input events (stage 15)

See [Deferred input queue (EN/RU)](doc/INPUT_QUEUE.md) and
[`examples/embedding/input_queue`](examples/embedding/input_queue).
This is a single-thread-owned FIFO, **not** a cross-thread queue and not yet
connected to MovieClip ActionScript handlers.

- [Native methods / Нативные методы AVM1](doc/AVM1_METHODS.md)
- [C89 object method example / Пример методов C89](examples/embedding/object_methods/README.md)

## AVM1 script receiver / Получатель скриптовых методов

See [bilingual API notes](doc/AVM1_SCRIPT_METHODS.md) and
[AS + C89 test example](examples/embedding/script_methods/README.md).
`CallMethod` now supports named script functions with `this` binding, but
first-class function properties and full Flash method lookup are pending.

## Table inspection (incremental)

See `doc/TABLES_DEBUG.md` and `examples/embedding/tables_debug/` for C89 table and debugger inspection APIs. Lua-like function environments, metaprotocol and stepping are **not yet implemented**.

### Internal containers and metatables

See [custom containers and metatables](doc/CUSTOM_CONTAINERS.md) for the
C++98 hash-table/vector implementation, C89 example and current limitations.

### Internal container replacements

The C++98 runtime uses private string/vector/hash-map implementations in `src/details/` instead of `std::string`, `std::vector` and `std::map`. See [English technical documentation](doc/INTERNAL_CONTAINERS.md) and [Russian translation](doc/INTERNAL_CONTAINERS.ru.md) for semantics, caveats, hybrid-table status and examples.

## Portable text helpers (stage 21)

See [Text API / Текстовый API](doc/TEXT_API.md) for the C89 UTF-8 decoder,
encoder, validation, offsets, and a deliberately limited byte-oriented regex
engine. A compilable example is in `examples/embedding/text/`. Source for the
regex subset lives under `external/`; bundled graphics/database dependency profiles are listed in
[external/README.md](external/README.md).

### Bundled dependencies and hybrid table

See [External dependencies](doc/EXTERNAL_DEPENDENCIES.md) for exact bundled
versus system-linked status, and the new standalone C++98 hybrid table.

### Stage 23: native metamethods

See [metamethod API](doc/METAMETHODS.md) and `tests/metamethods.c` for
`__index`, `__newindex`, `__iterate`, and explicit `__gc` finalization.
Automatic tracing GC and AS `pairs()` are not implemented yet.

### C89/C++98 stack extensions (stage 24)

The Lua-inspired stack bridge is described in [English](doc/STACK_API.md) and [Russian](doc/STACK_API.ru.md). Working examples: `examples/extending/stack_c/` and `examples/embedding/cpp98_stack/`. This is not a Lua-compatible ABI, and stack callbacks are not yet callable directly from AVM1.

### Native callable objects (stage 25)

See [callable API](doc/CALLABLE_OBJECTS.md) and
[the C89 example](examples/extending/callable_objects/README.md).
Native C callbacks can now be stored as context-owned object values and
invoked through the C API. AVM1 script closures and `__call` are not yet implemented.

## Native `__call` bridge (step 26)

`include/flashvm/meta_call.h` provides [`fvm_meta_call`](include/flashvm/meta_call.h#L20): native metatable callbacks receive the owner, defining metatable, and a new indexed argument table. See `tests/meta_call.c`. The proposed thread/channel language extensions are **not implemented**; design notes: `doc/THREADS_CHANNELS.md` and `doc/THREADS_CHANNELS.ru.md`.

### In-process channels (stage 27)

See [channels API](doc/CHANNELS.md) and [C89 example](examples/embedding/channels/README.md). A portable, synchronized FIFO channel is available to C and C++ embedding code. Worker VM execution and ActionScript channel operators are **not yet implemented**.

### Native worker threads

See [worker API](doc/WORKERS.md) and [C89 example](examples/embedding/workers/). Workers create isolated interpreter contexts through OS-specific `ports/` implementations. Opt-in ActionScript `thread.run` is documented in [ActionScript extensions](doc/ACTIONSCRIPT_EXTENSIONS.md).

Worker lifecycle API (suspended start, state snapshots, join):
[doc/WORKER_LIFECYCLE.md](doc/WORKER_LIFECYCLE.md).

### Cooperative worker cancellation

The portable C89 API supports cooperative worker stop requests and joining an
array of workers. See [worker cancellation](doc/WORKER_CANCELLATION.md) and
`examples/embedding/worker_stop/`. This is not yet the ActionScript `thread` API.

## Worker/channel exchange

A tested C89 example of cross-thread request/reply messaging is in [`examples/embedding/worker_channels/`](examples/embedding/worker_channels/). See [`doc/WORKER_CHANNELS.md`](doc/WORKER_CHANNELS.md) for blocking and nonblocking receives, borrowed string lifetimes, shutdown ordering and current limitations.

Bounded internal channel queues and backpressure: see [doc/CHANNEL_CAPACITY.md](doc/CHANNEL_CAPACITY.md).

Shared channel ownership is documented in [doc/CHANNEL_OWNERSHIP.md](doc/CHANNEL_OWNERSHIP.md).

### Owned channel handles (stages 34–38)

See [English API and limitations](doc/CHANNEL_VALUES.md),
[Russian API and limitations](doc/CHANNEL_VALUES.ru.md), and the C89/C++98
examples in `examples/embedding/channel_values`, `channel_cpp98`, and
`examples/extending/channel_worker`. GC-managed ActionScript wrappers are available through [`fvm_extensions_install`](include/flashvm/extensions.h#L17) and [`fvm_extensions_wrap_channel`](include/flashvm/extensions.h#L21).

## Garbage collection (experimental)

`flashvm/gc.h` exposes a context-local mark-and-sweep collector, native roots,
mode tuning and one-shot finalization. It can reclaim unreachable **cyclic**
object graphs. It is not yet integrated with AVM1 execution stack roots, so
collection must be invoked only at explicit safe points. See
[`doc/GARBAGE_COLLECTION.md`](doc/GARBAGE_COLLECTION.md) and
[`examples/embedding/gc/`](examples/embedding/gc/).
The script-level `gc.mode()` / `gc.state()` API is installed by [`fvm_extensions_install`](include/flashvm/extensions.h#L17).

## GC API references (stages 49–68)

See [GC safe points](doc/GC_SAFE_POINTS.md), [GC modes](doc/GC_MODES.md), [API index](doc/API_INDEX.md), and [GC troubleshooting](doc/GC_TROUBLESHOOTING.md). Active AVM1 frames are traced; automatic collection runs at instruction safe points.

GC active-frame roots: [English](doc/GC_ACTIVE_FRAMES.md) / [Русский](doc/GC_ACTIVE_FRAMES.ru.md).

### Native GC roots

See [temporary root scopes](doc/GC_NATIVE_ROOTS.md) for C89 embedding, ownership rules and current limitations.

### GC callback safety and diagnostics

See [GC callback safety](doc/GC_CALLBACK_SAFETY.md) for native-call
collection guards, root ownership and the versioned C89 diagnostic API.

### Automatic AVM1 GC safe points

The interpreter polls GC between instructions, with live VM frame roots
registered. See [GC safe points](doc/GC_AVM1_SAFE_POINTS.md) for native
ownership rules and limitations.

## Tested development update

See [ActionScript extensions](doc/ACTIONSCRIPT_EXTENSIONS.md), [collector contracts](doc/GC_COMPLETION.md), [concurrency ownership](doc/CONCURRENCY_SAFETY.md), and [validation](doc/VALIDATION.md). The update adds bounded finalization, userdata release, native capture roots, concurrent join safety, owned channel messages, cyclic graph snapshots and opt-in `gc`/`thread`/`channel` objects. Default builds require no external system library; SQLite is now bundled and enabled by DEFS.txt by default. Bundled regex sources remain in `external/`.

Without CMake on Linux, run `python3 scripts/check.py`; add `--sanitize` for ASan/UBSan. See validation for LeakSanitizer and platform limits.

File channels and value metatables: [English](doc/FILE_CHANNELS.md), [Russian](doc/FILE_CHANNELS.ru.md).
This checkpoint uses ABI 2; rebuild native clients. Test evidence and limitations: [English](doc/FILE_CHANNELS_CHECKPOINT.md), [Russian](doc/FILE_CHANNELS_CHECKPOINT.ru.md).

Shared mutexes and channel debugging: [English](doc/MUTEX_CHANNEL_DEBUG.md) / [Русский](doc/MUTEX_CHANNEL_DEBUG.ru.md).

Generated validation logs and summaries are excluded from Git. See [validation policy](doc/VALIDATION.md).

Resource descriptors over files/TCP, API and remaining limitations: [RU/EN](doc/RESOURCE_TRANSPORT.md).
Передача ресурсов через файлы/TCP, API и ограничения: [RU/EN](doc/RESOURCE_TRANSPORT.md).

Resource exceptions and SWF7–8 handlers / Исключения ресурсов и обработчики SWF7–8:
[English](doc/EXCEPTIONS.md), [Русский](doc/EXCEPTIONS.ru.md).

## Standard object graphs — v0.13 / Графы стандартных объектов

See [English](doc/STANDARD_OBJECTS.md) and [русский](doc/STANDARD_OBJECTS.ru.md)
for Object/Array/Date/Function, the host-authored MovieClip model, graph wire v2,
queue/file/TCP acceptance tests and explicit remaining completion gates.
This checkpoint does not certify complete simple types or MovieClip/SWF 8.

Property watches and enumeration / Наблюдение и перечисление: [documentation](doc/PROPERTY_WATCH.md).

Approved completion roadmap / Утверждённый план завершения: [doc/ROADMAP.md](doc/ROADMAP.md).

ABI 3 counted UTF-16 strings / Строки UTF-16: [UTF16_STRINGS.md](doc/UTF16_STRINGS.md).
Compatibility inventory / Реестр выполнения: [COMPATIBILITY_STATUS.md](doc/COMPATIBILITY_STATUS.md).

Generic Array and registered classes / Заимствованные методы и классы: [GENERIC_ARRAYS.md](doc/GENERIC_ARRAYS.md).

Source presentation checkpoint: see [English / Русский](doc/SOURCE_STYLE.md) for
scope, tutorial comments and searchable `[GC:]` lifetime annotations.

Function2 checkpoint: private registers, selected preloads, mapped named arguments
and graph transfer. See [English / Русский](doc/AVM1_FUNCTION2.md) for tested behavior
and remaining super/version limitations.

Tutorial diagnostics: examples use explicit checks that remain active under NDEBUG.
See [English / Русский](doc/EXAMPLE_CHECKS.md) for parameters and failure handling.

Class graph checkpoint: AVM1 Extends/ImplementsOp/CastOp, transitive interface
identity and graph wire 6. See [English / Русский](doc/AVM1_INHERITANCE.md).

Superclass checkpoint: named/preloaded super, receiver-preserving constructor and
method calls, live prototype depth and graph transport. See [English / Русский](doc/AVM1_SUPER.md).

SWF root action timeline checkpoint: [RU/EN](doc/SWF_MOVIECLIP_TIMELINE.md).
FWS/CWS frame actions and labels are imported into MovieClip; unimplemented scene/resource tags are rejected explicitly by this importer.

Sprite linkage checkpoint: [RU/EN](doc/SWF_SPRITE_RESOURCES.md).
DefineSprite/ExportAssets and attachMovie preserve independent instance timelines and a managed resource library.

MovieClip duplication checkpoint: [RU/EN](doc/MOVIECLIP_DUPLICATE.md).
Duplicates reset playback, retain resource linkage and own an independent copy of vector drawing state.

MovieClip depth encoding checkpoint: [RU/EN](doc/MOVIECLIP_DEPTHS.md).
Signed depths use biased internal slots; legacy snapshots retain their child identities when upgraded.

SWF placement/removal and affine/color checkpoint: [RU/EN](doc/SWF_SCENE_CONTROLS.md).
Sprite controls are supported; full SWF resource/display-list compatibility remains open.

Solid DefineShape/2/3 resource checkpoint: [RU/EN](doc/SWF_SOLID_SHAPES.md).

Linear/radial SWF and native-vector gradient checkpoint: [RU/EN](doc/SWF_GRADIENTS.md).

ActionScript gradient drawing checkpoint: [RU/EN](doc/MOVIECLIP_GRADIENT_FILL.md).

Point/Matrix checkpoint: managed flash.geom classes, numeric methods, captured
factory roots and queue/file transport. See [RU/EN scope and limitations](doc/GEOMETRY_CLASSES.md).

Rectangle checkpoint: bounds, derived Point properties, AVM1 construction and
queue/file/loopback TCP transport. See [RU/EN scope](doc/RECTANGLE.md).

ColorTransform checkpoint: native numeric channels, RGB, concatenation, private
state validation and graph transport. See [RU/EN scope](doc/COLOR_TRANSFORM.md).

Transform checkpoint: live MovieClip associations, local/composed typed snapshots,
world vector bounds and atomic copied color updates. See [RU/EN scope](doc/TRANSFORM.md).

AVM1 decomposition / Декомпозиция: value-based C services and prepared native script
bodies for future JIT reuse; file map, GC contract, tests and limitations:
[AVM1_DECOMPOSITION.md](doc/AVM1_DECOMPOSITION.md).

Raw prototype primitive retention / Raw-значения прототипа: [RU/EN](doc/OBJECT_RAW_PROTOTYPE.md).

Shared execution budget/GC/stop checkpoints / Общие контрольные точки: [RU/EN](doc/EXECUTION_CHECKPOINT.md).

Loaded numeric text and strict NaN / Числовой текст и строгий NaN: [RU/EN](doc/AVM1_NUMBER_TEXT.md).

Named uppercase constants / Смысловые макросы: [header map and evidence](doc/SEMANTIC_CONSTANTS.md).

Experimental explicit native compilation is now available in v0.348; see
[JIT and interpreter usage](doc/JIT.md). Earlier checkpoint sections are historical.

Runtime interpreter/JIT options, switching during execution and opt-in diagnostic
interpreter replay are available in v0.348. See [execution modes](doc/EXECUTION_MODES.md)
and run `flashvm_examples_embedding_execution_modes_main` after building it.

Multi-architecture JIT, breakpoints, threads and multiple VM examples / JIT для разных CPU, отладка, потоки и несколько VM: [JIT_ARCHITECTURES.md](doc/JIT_ARCHITECTURES.md).

Native DefineFunction/DefineFunction2: [English](doc/JIT_DEFINITIONS.md) /
[Русский](doc/JIT_DEFINITIONS.ru.md); captured scope/pool, callbacks and allocation failures.

Native With: shared operands/registers/pool, nested Return and scoped cleanup;
[English](doc/JIT_WITH.md) / [Русский](doc/JIT_WITH.ru.md).

Native Try/Catch/Finally: retained exceptions/returns, register/name catches and
root OOM containment; [English](doc/JIT_TRY.md) / [Русский](doc/JIT_TRY.ru.md).

Borrowed negative splice / отрицательная длина: [EN](doc/ARRAY_NEGATIVE_SPLICE.md) / [RU](doc/ARRAY_NEGATIVE_SPLICE.ru.md).

Signed borrowed end operations / знаковые заимствованные методы: [EN](doc/ARRAY_NEGATIVE_ENDS.md) / [RU](doc/ARRAY_NEGATIVE_ENDS.ru.md).

Negative loaded slice / slice с отрицательной длиной: [EN](doc/ARRAY_NEGATIVE_SLICE.md) / [RU](doc/ARRAY_NEGATIVE_SLICE.ru.md).

Large loaded Array mutations / большие загруженные массивы: [EN](doc/ARRAY_LOADED_LARGE_MUTATION.md) / [RU](doc/ARRAY_LOADED_LARGE_MUTATION.ru.md).

Large loaded sorting / сортировка больших массивов: [EN](doc/ARRAY_LOADED_LARGE_SORT.md) / [RU](doc/ARRAY_LOADED_LARGE_SORT.ru.md).

Unicode legacy display paths / Unicode-пути старых SWF: [EN](doc/MOVIECLIP_UNICODE_PATHS.md) / [RU](doc/MOVIECLIP_UNICODE_PATHS.ru.md).

[Live function interfaces](doc/REMOTE_FUNCTIONS.md) — v0.474.

[Function RPC examples: processes, independent VM threads and LAN](doc/FUNCTION_RPC_EXAMPLES.md).

[Automatic CMake development-version refresh](doc/BUILD_VERSION.md).

[Loaded Object.__resolve](doc/OBJECT_RESOLVE.md) — v0.474: missing-property
callbacks with counted names, versioned lookup, GC roots and recursion bounds.

[C89 resolver tutorial](examples/embedding/property_resolve/README.md).

[Loaded builtin construction / Конструирование и вызовы SWF](doc/BUILTIN_CONSTRUCTION.md) — v0.474.

[Current-global primitive boxing / Текущие классы примитивов](doc/PRIMITIVE_GLOBAL_BOXING.md) — v0.474.

Loaded Function calls and native construction values: [RU/EN contract](doc/FUNCTION_CONSTRUCTION.md).

ASSetPropFlags conversion order and supported flags: [RU/EN](doc/PROPERTY_FLAGS.md).

Loaded Math namespace attributes and host activation: [RU/EN](doc/MATH_ATTRIBUTES.md).

Debugger tutorials / Примеры отладчика: [examples/debugger](examples/debugger/README.md).
Complete API reference: [English](doc/DEBUG_API.md) / [Русский](doc/DEBUG_API.ru.md);
[SWF/SWD source mapping](doc/SWD_DEBUGGING.md).

Полный каталог примеров / Complete example catalog: [examples/README.md](examples/README.md).
Documentation source links and maintenance / Правила документации: [RU/EN](doc/DOCUMENTATION_GUIDELINES.md).
Stage quality / Качество сцены: [API and ToggleQuality](doc/STAGE_QUALITY.md).
[Multimedia and device bridges / Мосты мультимедиа и устройств](doc/MULTIMEDIA_BRIDGES.md): static FFmpeg and replaceable host callbacks.
[Native speech / Системные TTS и STT](doc/SPEECH_BRIDGES.md): explicit providers and per-OS prepared-host examples.
## License

FlashVM's original code and documentation are licensed under the
[MIT License](../LICENSE). Copyright (c) 2026 bhgv.
Third-party code, data and dependencies retain their own copyright notices and
licenses; the root MIT license does not replace them. See notices in
[external/](external/README.md) and the relevant dependency directories,
including any third-party material identified within project sources.
v0.583: independent native audio worker and [prepared StopSounds](doc/AUDIO_STOP_BINDINGS.md); Android crackling remains under investigation after the device report.

v0.584: [Android PCM/WAV diagnostics / диагностика звука](doc/PLAYER_AUDIO_DIAGNOSTICS.md).

v0.586: [negotiated SDL audio block and reserve](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0586-publish-the-negotiated-device-quantum).

v0.587: [optional stop release / затухание остановки](doc/PLAYER_AUDIO_RELEASE.md).

v0.588: [ADPCM test-tone block restart fix / исправление тестового ADPCM-тона](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0588-adpcm-fixture-block-restarts).

## Browse documentation locally

Run `python -m pip install -r doku_viewer/requirements.txt`, then
`python view_docs.py`. Python 2.7.9/3 server, browser Markdown navigation and
Pygments source-line links: [viewer guide](doku_viewer/README.md).
## Desktop application

[Desktop frontend](frontend/README.md): optional Linux/Windows/macOS SDL2 player,
command-line SWF paths, JIT/interpreter, quality, FPS/percentage timing and help.
[Full guide](doc/DESKTOP_FRONTEND.md).
Incremental StartSound mixer: [API guide](doc/SWF_SOUND_MIXER.md) /
[RU](doc/SWF_SOUND_MIXER.ru.md), bounded independent voices and explicit PCM reads.
Root timeline sound delivery: [EN](doc/MOVIECLIP_SOUND.md) /
[RU](doc/MOVIECLIP_SOUND.ru.md); actual frame entry before DoAction,
source checks and host-local callback registration.
Explicit mixed-rate/channel audio: [EN](doc/SWF_SOUND_CONVERSION.md) /
[RU](doc/SWF_SOUND_CONVERSION.ru.md); configured output including 48000 Hz stereo,
with legacy strict API preserved and an annotated --convert-demo tutorial.
[Auditable native release evidence](doc/RELEASE_EVIDENCE.md).
[Build for the host or another OS/CPU](../BUILDING.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](include/flashvm/flashvm.h#L74) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:38](include/flashvm/avm1.h#L38) |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:17](include/flashvm/extensions.h#L17) |
| `fvm_extensions_wrap_channel` | function | [include/flashvm/extensions.h:21](include/flashvm/extensions.h#L21) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](include/flashvm/flashvm.h#L72) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](include/flashvm/meta_call.h#L20) |
| `fvm_register_function` | function | [include/flashvm/value.h:101](include/flashvm/value.h#L101) |
| `fvm_swf_execute_actions` | function | [include/flashvm/avm1.h:73](include/flashvm/avm1.h#L73) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->

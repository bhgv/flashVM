[Moving windows and independent Close/Cancel](DESKTOP-LIFECYCLE.md)

[MovieClip desktop / independent Limbo processes](DESKTOP.md)

[Explicit binary/debug export — Ctrl+F7 / Ctrl+Shift+F7](ARTIFACT-EXPORT.md)

[F7 Build / F8 diagnostics — v0.1671](BUILD-PANEL.md) · [Русский](BUILD-PANEL.ru.md)

# FlashVM Limbo IDE and host build tools

Version **0.7.1**. [Русский](README.ru.md).

**Target application:** a Limbo IDE running in an embedded Dis runtime inside FlashVM, with channel-only UI control. This Python driver is a host development tool. [Accepted architecture](ARCHITECTURE.md).

This is an independent Python 3.9+ host build backend for an editor, not a graphical IDE. The explicitly selected C99 source compiler, FIR tools and SWL remain independent projects. Current source coverage is their numeric AS1/AS2/AS3/Limbo profile; AS3 produces AVM1 compatibility code, not ABC. No complete AS2 compiler is claimed.

[Channel-owned directory browser](dis/DIRECTORIES.md) / [Каталоги через каналы](dis/DIRECTORIES.ru.md).

## Run

Build the tools using the [FIR pipeline guide](../fir/v1/doc/PIPELINE.md). Then supply absolute or current-directory-relative executable paths:

```sh
python3 ide/build.py program.as movie.swf --language as2 \
  --compiler /host/build/fir1_source --ir /host/build/fir1 \
  --linker /host/build/swl --module application
```

The driver emits one JSON object to stdout for a parsed request; exit 0 means publication, 1 means a returned build failure. argparse help/invalid options use ordinary help text and exit 0/2. Tools run without a shell or stdin; spaces, Unicode and shell metacharacters in source/output paths are ordinary path characters. These must be trusted host executables, never cross-built target binaries. Nothing is downloaded or built implicitly.

Optional `--runner /host/flashvm_runner --expect-trace TEXT` gates publication on successful headless execution and exact stdout. In a shell the expected trace must contain a real newline, for example a quoted multiline string; a literal backslash-n is not converted. `--no-optimise` omits optimization and its second verifier; `--timeout 30` selects a finite positive per-stage limit.

## Transactions and process ownership

The build reads a bounded 64 KiB source snapshot before compilation. A private directory beside the output owns intermediate FIR, optimized FIR, SWO and SWF files. Each child gets this directory as cwd, TMPDIR, TEMP and TMP; invalid ambient temporary paths cannot break compiler staging. Source, verifier, optimizer, verifier, object emitter and linker run serially. No later stage runs after a failure. Signature checks require SWO1 and uncompressed SWF8 with matching declared length; this is basic artifact admission, not full SWF validation.

Only after every requested stage succeeds does os.replace publish the movie on the same filesystem. Existing output survives all prepublication failures, including trace mismatch. Outputs aliasing the source or a tool by path/hard link, and symbolic-link output entries, are rejected. Concurrent builds targeting the same output have last-successful-writer semantics; the backend does not lock them. Atomic replacement is not a power-loss durability guarantee and depends on filesystem semantics. Input may be a symlink; diagnostics identify its resolved path.

Each direct child is killed and reaped on timeout/cancellation. Descendant process groups are not managed; supplied compiler tools should not detach children. KeyboardInterrupt becomes a returned cancellation diagnostic. No file handles or child processes transfer to the caller. Python exception types outside the documented request/OS failures may still propagate to an embedding host.

## JSON and Python API

The synchronous [`build`](build.py) function returns caller-owned Python data. It does not mutate environment globals. Result fields: schema=1, ok, published, steps, diagnostics; accepted inputs add canonical source/output, language and module, and a successful publication includes sha256. Each step records stage, argv, returncode, bounded stdout/stderr and truncation flags; timeout adds timed_out. Logs keep at most 65536 bytes per stream in the result, with replacement decoding for invalid UTF-8. Temporary log files are not disk-quota bounded. JSON contains local executable and temporary paths, so the caller decides where to retain/share it.

Diagnostic fields are severity, stage and raw message. Source messages with source:line:column or source line N gain reported coordinates and the original resolved source file. The driver invents no coordinates for other messages. Source-line-only semantics keep column absent. Stages are input, source, verify, optimise, verify_optimised, object, link, run and publish. Build stages before execution may have a diagnostic without a completed step record.

## Verification and limits

```sh
python3 -m unittest discover -s ide/tests -v
python3 ide/tests/integration.py --compiler /host/fir1_source --ir /host/fir1 \
  --linker /host/swl --runner /host/flashvm_runner
```

Unit fixtures use deterministic process doubles and test orchestration, not compiler semantics. Integration requires real tools and checks optimized/unoptimized execution plus rollback after syntax and trace failures for all four numeric profiles. Tests and the driver work after the IDE directory is copied/renamed; only the shared MIT LICENSE must accompany a detached distribution. Windows, physical Android and desktop UI integration remain separate acceptance work. Editor buffers, incremental compilation, source maps, project files and debugger UI are not implemented here.

[Numeric example](examples/numeric/README.md) · [Validation / Проверки](VALIDATION.md).

[Human-readable code rules](AGENTS.md): comments before functions/types, explicit channel ownership, GC and JIT/debug invariants.

[AS1/AS2 ↔ Dis module interface](MODULE_INTEROP.md): planned bidirectional module support; later AS IDE port is allowed.

## Executable IDE component

The [portable document core](dis/README.md) is implemented in Limbo and builds to
`idecore.dis` plus `.sbl`. It is an executable library component, not a full IDE or
an embedded FlashVM runtime. [Typed channel protocol](dis/PROTOCOL.md).
The deployed IDE must be interpreted/JIT bytecode (Dis and/or SWF); this Python
backend remains host tooling. Combined SWF/Dis packaging and extra SWF tags are
explicitly deferred. Current module delivery does not depend on that decision.

[Limbo navigation/search tutorial / Навигация и поиск](dis/examples/navigation/README.md) · [Channel contract / Контракт каналов](dis/PROTOCOL.md).

[Revision-bound Undo/Redo / История правок](dis/HISTORY.md) ·
[Runnable Limbo history example / Пример](dis/examples/history/README.md) ·
[Validated source diagnostics / Диагностика](../fir/v1/doc/SOURCE_DIAGNOSTICS.md).

`--structured-diagnostics` opts into the FIR 0.5.4 schema. Accepted locations
include original byte spans and Unicode-character offsets; `source_sha256`
identifies the captured snapshot. Compare it and the live revision before using
late results. Without the option, legacy compiler messages remain supported.

[Own compiler priority / Приоритет Limbo](../compiler/limbo/OWN_COMPILER.md) ·
[Two-way .m external AS/Dis modules / Внешние модули](../compiler/limbo/examples/external-modules/README.md) ·
[Owned tools / Инструменты](../tools/UPSTREAMS.md).

[Bulk replacement, one Undo / Массовая замена и один Undo](dis/REPLACE_ALL.md)

[Series 0.5.0 validation / Проверки серии](dis/SERIES_0_5_0.md)

[Editor services / Сервисы редактора](EDITOR-CHANNELS.ru.md).

## Graphical editor v0.1550

[Executable tutorial and limitations](dis/examples/graphical-editor/README.md):
Limbo controller, actual Dis runtime, channel-owned document/history/file I/O and
FlashVM TextField rendering. Build component `disvm-editor`, launch menu item 8.
AS-worker execution and debugger integration remain open.

## Editor series v0.1551–v0.1554 / Серия редактора

[Navigation](dis/NAVIGATION.md) · [Viewport](dis/VIEWPORT.md) ·
[Actions](dis/ACTIONS.md) · [Noto/zoom](dis/ZOOM.md).
The current graphical package includes default Noto Sans Mono/11 without a font
question, Dis-owned scrolling/mouse/search/file actions and Ctrl-wheel/pinch zoom.

[Executable Limbo extension status: tables, projections and any](../fir/v1/doc/EXTENSIONS.md).

[IDE application on our DisVM / Приложение IDE на нашем DisVM](dis/app/README.md) · [Русский](dis/app/README.ru.md).

[Graphical file browser](dis/GRAPHICAL-BROWSER.md) / [Графический браузер файлов](dis/GRAPHICAL-BROWSER.ru.md).

[AS2 → FIR → SWO → SWF view](../compiler/as2/view/README.md) · [Русский](../compiler/as2/view/README.ru.md).

[Dis channels, AS and parallel native rendering](RENDER_OWNERS.md) / [Русский](RENDER_OWNERS.ru.md).

[Native compiler jobs / Библиотечные сборки на Dis](COMPILER-SERVICE.md) · [Русский](COMPILER-SERVICE.ru.md): actual Coco/R/FIR/SWL libraries, revision-bound workspace worker, copied diagnostics and artifacts. F7 Build/F8 diagnostics now use the portable revision-bound panel; Run/Debug remain future work.

[Mouse caret and selection](DESKTOP-POINTER.md).

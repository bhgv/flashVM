# IDE build backend

Version **0.2.3**. [Русский](README.ru.md).

**Target application:** a Limbo IDE running in an embedded Dis runtime inside FlashVM, with channel-only UI control. This Python driver is a host development tool. [Accepted architecture](ARCHITECTURE.md).

This is an independent Python 3.9+ host build backend for an editor, not a graphical IDE. The explicitly selected C99 source compiler, FIR tools and SWL remain independent projects. Current source coverage is their numeric AS1/AS2/AS3/Limbo profile; AS3 produces AVM1 compatibility code, not ABC. No complete AS2 compiler is claimed.

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

The synchronous [`build`](build.py#L103) function returns caller-owned Python data. It does not mutate environment globals. Result fields: schema=1, ok, published, steps, diagnostics; accepted inputs add canonical source/output, language and module, and a successful publication includes sha256. Each step records stage, argv, returncode, bounded stdout/stderr and truncation flags; timeout adds timed_out. Logs keep at most 65536 bytes per stream in the result, with replacement decoding for invalid UTF-8. Temporary log files are not disk-quota bounded. JSON contains local executable and temporary paths, so the caller decides where to retain/share it.

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

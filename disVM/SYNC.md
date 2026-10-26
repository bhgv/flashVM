# Current synchronization — v0.1626

Priority: x86-64 and ARM64. Experiment: `875d12263efa72bab84d27f0d4215713e7da9465`.
Imported cc51c63 adds five scalar-channel constructor paths: NEWCB/NEWCW/NEWCL/
NEWCF/NEWCP. AMD64/ARM64 availability is 160 of 173 active opcodes, not complete
operand coverage. RV64 code is unchanged in this series.

Our returned TCMP fix guards backward Type-field addresses in the C interpreter
and both priority JITs. Small nonzero owners previously underflowed the address
or read the Type field at nil; UBSan reproduces the defect against the parent.
Native decline preserves the VM image and performs no ownership/GC changes.
The generic emulation fixture also includes the new channelentry ABI field.

[Scalar channels](upstream/libinterp/tests/README.scalar-channel-native.md) ·
[TCMP contract](upstream/libinterp/tests/README.typecheck-prefix.md).
453 exact core/reference files and 174 bootstrap files are selected. No table
language extension was exported to Inferno. ARM64 instruction emulation does not
establish physical hardware execution. Historical reports follow.

---

# Current synchronization — v0.1625

Selected experiment: `b2da16226dfc5bc0e0c754ae5d95fcde6cc1afb9` (three commits after b9a569e).
The Dis snapshot contains 446 byte-identical files; the temporary bootstrap has 174.

AMD64/ARM64 now include native ICONSM/ICONSP list payload construction. Raw
copies stage destination ownership until allocation and copying succeed; pointer
payloads retain and mark current owners after the allocator returns. Copies above
256 bytes preserve the checked interpreter fallback. RV64 availability is unchanged.
Strict heap-prefix guards reject the exact boundary that would access address zero.
See [payload ownership and test contracts](upstream/libinterp/tests/README.payload-cons-native.md).

Compiler synchronization includes bounded source-history growth and overflow-safe
coordinate/case lookup, with production fixed-point selection and cast/string
folding regressions. These are imported compiler references and bootstrap fixes;
our Coco/R frontend and its native table Dis backend remain independent.

Seven acceptance drivers are now registered: payload layout, AMD64 native payload,
AMD64/ARM64 emulation, source history, fixed selection, cast folding and string folding.
WORD32/WORD64 layout models and ARM64 instruction emulation do not establish a
physical ARM64 port. No table-language extension is exported to Inferno.

Earlier synchronization reports follow as history.

---

# Current synchronization / Текущая синхронизация — v0.1593

Selected experiment: `74c193724a7352b8fda2724a5cf10029885be68c`.
Includes unique array-view SLICEA leaves and self-load ownership from 4d9ce2e,
plus our returned Loader construction/scalar result-slot fix.
[Module audit](libraries/MODULE_AUDIT.md) · [Upstream Loader contract](upstream/libinterp/tests/README.loader-results.md).
Vendored files remain byte-identical; native ARM64/RISC-V platform execution
is not established by host/emulator fixtures. Earlier reports follow as history.

---

# Dis, JIT and Sys refresh — v0.1540

[Полный русский отчёт](SYNC.ru.md) · [Manifest](UPSTREAM.json)

Source: https://github.com/bhgv/Inferno-OS-bhgv, **experiment**.
Pinned commit: `f948343b711c8300dbf76417254c18a7a8d2f5ff`, eight commits after
previous Dis snapshot `24892373ee09a2a23dc39b9dedc01fb7af61d8c1`.

## Imported and adapted

AMD64/ARM64 real arithmetic, powers/conversions/rounding, signed fixed scales
and minus-one division; emu/os Sys FD ownership/publication and unaligned
connection/result slots; process/heap diagnostics. The native Limbo bootstrap
also retains divisor evaluation and division-by-zero traps when fixed scaling
would otherwise lower to a constant zero.

The selected snapshots contain 192 Dis/reference-test files and 174 temporary
bootstrap files. Original blobs, notices and SHA-256 hashes are retained.
The new devprog bodies and two compiler source files support extracted
regressions only. No whole Inferno, GUI, Math or Geometry module was imported.
The project's Coco/R compiler remains the priority.

CMake stays compatible with 3.10; the Dis component version is now 0.1.4.
Eleven new production regressions and eight foreign-emitter build checks are
registered. TLS register/safepoint generation and the FlashVM owner bridge retain
their ABI. The Sys module header is unchanged; its implementation was refreshed.

## Executed validation, Linux AMD64

- Core build and regressions: 42/42.
- TLS/FlashVM bridge/GC tests plus foreign-emitter checks: 62/62.
- Eight focused Sys/diagnostic drivers with UBSan: 8/8.
- Source-built Limbo bootstrap product test: 1/1.
- Six IDE service modules and their .sbl files rebuilt with that bootstrap.
- Both provenance manifests verified.

Native AMD64 execution, WORD-width fixtures and ARM64/RV64 emitter construction
are distinct evidence. No ARM64/RV64 execution or Windows/macOS/Android runtime
acceptance is claimed. Sys tests inject OS/scheduler boundaries around actual
production bodies. The complete executable Dis host, Sys/VFS registration and
IDE startup remain unfinished. No newly confirmed upstream defect was found.

See the Russian report for source selection, repeat commands and integration limits.

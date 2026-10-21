# Aggregate prepared-stream budget in the player

[Русский](PLAYER_STREAM_BUDGETS.ru.md) · [native memory API](STREAM_STORAGE.md) · [playback](PLAYER_PCM_STREAMS.md)

The unmuted desktop player now applies one prepared-source ceiling across its
whole root header sequence. The default is 64 MiB (67108864 bytes), declared as
[FVM_DESKTOP_AUDIO_PREPARED_BYTES](../frontend/common/audio_memory.h#L5) in the SDK-independent
[audio memory policy header](../frontend/common/audio_memory.h). The existing
64-header limit is separate. Console help prints the value from this same policy
header, rather than an unrelated hard-coded description. Public player-host and
VM structure layouts remain unchanged; there is no new command-line option.

The charged group includes the allocated [FVM_DesktopStreamSegment](../frontend/common/audio.h#L26) table, every
retained snapshot object/index/payload, and the temporary encoded-plus-decoded
ownership of the next ADPCM snapshot. Before preparing source `i`, the driver
requires `table + retained(previous sources) + peak(source i) <= ceiling`.
After preparation it subtracts only retained bytes; compressed temporary storage
has already been released. Empty snapshots still have an object/index charge.
The native [bounded preparation and storage APIs](STREAM_STORAGE.md) enforce the
per-snapshot part before allocation; runtime payload limits remain independent.

## Preflight, acquisition and cleanup

[fvm_desktop_audio_validate](../frontend/common/audio.h#L76) performs the same arithmetic in source order before
window/device acquisition. It temporarily prepares **PCM as well as ADPCM**, queries
charges, closes each temporary and virtually retains its charge in the remaining
budget. Thus a sequence whose completed snapshots fit, but whose later temporary
expansion would overflow the ceiling, is rejected before opening native endpoints.
First failing validation/budget/allocation status propagates. OUT_OF_MEMORY is a
preparation failure, not a claim that a codec or the whole SWF format is unsupported.

Actual audio acquisition checks header policy, charges the table before `calloc`,
and repeats preparation using the remaining group budget. The returned snapshots
are retained until worker join. On later failure no device/worker has been opened;
the caller's ordinary close releases all already-owned entries and the table.
Allocator failures after successful preflight can still occur during actual
acquisition; normal error cleanup applies. No hidden preflight cache survives.
Copying/indexing/ADPCM decoding is deliberately repeated at startup. No accounting,
allocation, decode or VM access is added to the native audio producer.

The cap concerns **prepared stream ownership**, not total process RSS. Original
SWF/VM memory, allocator bookkeeping, stack/retry buffers, renderer resources, SDL
and event-sound voice ownership are excluded. The table is counted here although
it is excluded from the native snapshot API's own observation. The cap does not
change 9P `msize`, S-record slot negotiation, source transport limits or frame gates.
Mute creates no prepared snapshots and does not apply this group ceiling; existing
core structure/PCM-alignment validation still runs. Event/stream mixing and other
unsupported stream formats retain their existing explicit policy.

For custom C/C++ applications the core bounded API accepts a runtime per-call
budget, independent of CLI parsing. The shipped frontend uses its compiled policy;
private build-time overrides must be consistent across its audio/help sources and
need separate platform validation. The standard build and small test-only override
are the configurations checked here.

## Examples and independent checks

The [player PCM/ADPCM examples](../examples/player/pcm_stream/README.md) keep the
same files and commands; the ordinary default budget covers them. The standalone
[C89 stream exporter](../examples/multimedia/swf_stream/README.md) demonstrates an
explicit one-MiB ceiling and `--storage` observations after input VM teardown.
Neither example requires a new codec library or console-bound memory manager.

[stream_budget.cpp](../frontend/tests/stream_budget.cpp) builds the real driver with a 64-KiB test-only ceiling. Three
whole stereo ADPCM sources fit; a fourth fails before device/worker creation.
A PCM predecessor plus a two-packet ADPCM source has final retained ownership below
the cap but a simultaneous expansion peak above it. The test derives both charges
from the native API, requires preflight/acquisition to agree and closes partial
ownership before VM teardown. It is an allocation/ownership test with dummy SDL,
not physical-device acceptance.

For static player-host builds, `flashvm_frontend_stream_budget_player` links that
same small driver and console-options policy into the actual CLI. The Python
[stream_budget.py](../frontend/tests/stream_budget.py) checks its advertised ceiling and successful ordinary playback,
then supplies unavailable video/audio drivers for retained, temporary-peak and PCM
excess cases. Each must fail at root audio policy before a window/device opens.
Mute passes all these valid encoded movies without an audio device. This fixture
is not installed as the release player; shared-host builds use the direct C++
small-driver fixture alongside ordinary shared-player regression instead.

```sh
cmake --build build --target flashvm_frontend_stream_budget flashvm_frontend_stream_budget_player
ctest --test-dir build --output-on-failure -R '^(flashvm_frontend_stream_budget|flashvm_player_stream_budget)$'
```

Use the [normal player build prerequisites](PLAYER_TEST_BUILDS.md). The small CLI
target requires tests, a static player-host build, desktop frontend and SDL audio;
its Python harness is omitted for cross builds. Other OS/CPU runtime acceptance
remains a separate stage-9 requirement.

## Validation scope

111 selected native and 24 shared-host checks passed, including the small-budget
fixtures, actual CLI, ordinary PCM/ADPCM sample oracles, offscreen embedding and
resource channels. Edited modules passed ASan/UBSan with default and 64-KiB driver
variants; frontend allocation failures were isolated through budget 11. Core
preparation/storage faults were checked separately through budget 4. Full VM/SDL
instrumentation and process leak enumeration were not available. The pre-existing
Windows speech comment-checker failures documented in STREAM_ADPCM.md are unchanged;
a successful global multimedia comment check is not claimed.

Complete minimal cores plus storage/C89/export executables linked for ARM64,
RISC-V64 and x86-64 Linux/musl. The updated static ARM64 SDL/AGG player linked;
all thirteen selected ELF executables passed CPU/static-load checks. The x86-64/musl
storage, allocation and C89 tests and complete C tutorial actually ran on this
host, including the independent PCM oracle. No ARM64/RISC-V64 execution or physical
speaker/window acceptance is claimed. Platform acceptance and all roadmap stages
remain open; generated logs/SDKs/caches stay outside tracked source.

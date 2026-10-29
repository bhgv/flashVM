# Player compatibility and input preflight

Current root stream/event composition and its remaining gate/pause/device limits: [desktop mix contract](DESKTOP_MIX.md). Earlier versioned stream-only statements below are historical.

[Русский](PLAYER_PREFLIGHT.ru.md).

Current v0.761 policy: [bounded PCM header sequences play](PLAYER_PCM_STREAMS.md); unsupported codecs/mixed/excess header counts still fail before native setup. The section below records the original muted-import contract.

## SWF5 cursor namespace (v0.662)

The player now installs Mouse for SWF5–8, matching the existing
[fvm_standard_mouse_install](../include/flashvm/mouse.h#L66) contract. Mouse.hide/show return the previous requested
visibility. addListener/removeListener remain unsupported for loaded SWF5; installing
the namespace does not bypass that version gate. SWF1–4 keep the earlier path.
The SDL owner-thread cursor policy and teardown restoration are unchanged.

flashvm_player_legacy_cursor_interpreter/jit execute actual SWF5–8 movies that
check hide/hide/show/show returns 1/0/0/1. Deliberately wrong values fail, and a
separate SWF5 listener call must fail while SWF6–8 listener calls succeed. Dummy
video checks script/runtime behavior, not a physical cursor or complete Flash API.
## Explicit root-stream muting (v0.663)

Root streaming sound is validated before window/device setup. Originally all unmuted root streams were rejected; v0.758 permits the bounded
single-PCM policy linked above.
With --mute the player uses the additive [fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L216) API and
[FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../include/flashvm/movieclip.h#L204). It validates and omits only root stream controls
from scene playback. The original [fvm_movieclip_load_swf](../include/flashvm/movieclip.h#L129) remains strict.

The opt-in requires its decoded input to match the current [fvm_load_memory](../include/flashvm/flashvm.h#L90) source
byte-for-byte. Validation therefore covers the exact imported root: stream header
order/version, frame membership, duplicate blocks and PCM alignment. Unknown flag
bits fail; output is null on failure. Runs no actions and opens no device. Host
must root the returned clip/library; owner-thread/GC restrictions are unchanged.
Compressed bytes remain opaque: this does not add MP3 decoding or A/V sync.
Sprite streams remain unsupported. Event sounds retain their separate host callback;
--mute installs the existing muted event callback as before.

Tests cover FWS/CWS PCM, opaque MP3, mixed event/stream sound, malformed headers,
orphan/duplicate/odd PCM blocks, strict versus opt-in import, source mismatch,
unknown flags, nested-stream rejection and a pinned clip across GC/advance.
## Complete measured file input (v0.664)

After reading the measured file length the host probes EOF. Growth is rejected
rather than silently playing a prefix; a short read, EOF-probe error or close
failure also aborts. Stale unrelated SDL error text is cleared before the probe.
Every acquired stream closes once; failure keeps the caller's size output
unchanged and frees the input buffer. The 64 MiB positive/seekable input bound
is unchanged. This detects length changes, not concurrent same-length rewrites;
callers must still supply stable files. UTF-8 platform file access stays in SDL.

The input-reader fixture substitutes real SDL memory streams for file opening,
then controls advertised size and error injection. It checks normal EOF, stale
errors, growth, truncation, invalid sizes, open/probe/close failures and cleanup
without initializing a window or device.
## Locale-independent numeric options (v0.665)

The ASCII --fps/--speed parser no longer calls strtod or changes process locale.
A C host can invoke it after selecting a comma-decimal locale without breaking
`--fps 12.5` or changing other threads' locale. Decimal points and optional e/E
exponents remain supported; a trailing % is allowed only for speed. Signs are
allowed only on the exponent. Input is bounded to 128 bytes and exponent magnitude
400; invalid syntax and values outside existing FPS/percentage limits fail.
Conversion uses finite double presentation arithmetic, not an exact decimal or
ActionScript number-conversion contract. Extremely long numeric arguments are not
accepted. Frame limits retain their checked unsigned-integer path.

The standalone options fixture poisons strtod and setlocale and verifies neither
is called, then checks decimal/exponent forms, comma/garbage rejection, bounds
and the speed suffix. It is a deterministic dependency/grammar test, not a fresh
runtime validation of an installed foreign locale or every floating-point target.
## C interface details and tutorial (v0.666)

[fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L216) receives the owning context, borrowed complete FWS/CWS
bytes and exact size, a borrowed clip name, flags, and mandatory object-output
pointer. Install the standard object model first. The byte/name inputs need only
outlive the call; imported storage is owned by the context. The returned object
is borrowed managed storage, so the host must pin/root it before collection.
The function runs synchronously on the owner thread outside collection/teardown,
performs no script/device callbacks, and does not change the canonical loaded SWF.
Zero flags delegate to the same strict import behavior as the original API.

[FVM_OK](../include/flashvm/flashvm.h#L44) returns a clip; malformed input, mismatched canonical source or unknown
flags return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47); remaining unsupported scene features (including
sprite streams) return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48); allocation failure returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L46).
If out itself is null there is no output to clear; otherwise every failure leaves
*out null. Opaque compressed stream bytes are not semantically decoded or certified.
Event-sound callback ownership and timing are unchanged.

[Runnable C89/GC and SWF8 tutorial](../examples/player/muted_stream/README.md)
shows complete setup, exact-source import, input release, pin/GC/advance and cleanup.
Its expected scalar output is `streams=1 advanced=1 muted=1`. It builds without
SDL; the separate player command demonstrates visible/dummy scene playback.
The source generator reproduces the small SWF without sibling implementation paths.

Validation uses Linux x86-64/GCC 13.3 and external SDL2 2.30.12. The affected player
suite plus targeted sound/scene/stream tests are run; no full core/sanitizer suite
is claimed. Full AGG/JIT player builds link with separately built target SDL SDKs
for Linux ARM64, Linux RISC-V64 and Windows x64 under the existing documented Zig
host-wrapper adaptation. These are cross builds, not foreign OS/device execution.
Android/macOS, audible streaming, A/V sync, physical devices, endurance and all
nine roadmap stages remain open. [Build guide](../../BUILDING.md),
[SDK provenance](PLAYER_RECOVERY.md#validation-of-this-series).
All 59 selected native tests pass (44 frontend/player plus 15 sound/scene/stream).
The new import is also checked at every injected new/new[] boundary; collector,
release and teardown states reject entry before temporary decoding allocations.
The standalone C89 muted-import host also cross-links for all three targets;
its ELF/PE machine headers were verified. It was executed only on native Linux.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../include/flashvm/flashvm.h#L90) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:129](../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:216](../include/flashvm/movieclip.h#L216) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:66](../include/flashvm/mouse.h#L66) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:204](../include/flashvm/movieclip.h#L204) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->

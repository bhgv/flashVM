# Player compatibility and input preflight

[Русский](PLAYER_PREFLIGHT.ru.md).

## SWF5 cursor namespace (v0.662)

The player now installs Mouse for SWF5–8, matching the existing
[fvm_standard_mouse_install](../include/flashvm/mouse.h#L50) contract. Mouse.hide/show return the previous requested
visibility. addListener/removeListener remain unsupported for loaded SWF5; installing
the namespace does not bypass that version gate. SWF1–4 keep the earlier path.
The SDL owner-thread cursor policy and teardown restoration are unchanged.

flashvm_player_legacy_cursor_interpreter/jit execute actual SWF5–8 movies that
check hide/hide/show/show returns 1/0/0/1. Deliberately wrong values fail, and a
separate SWF5 listener call must fail while SWF6–8 listener calls succeed. Dummy
video checks script/runtime behavior, not a physical cursor or complete Flash API.
## Explicit root-stream muting (v0.663)

Root streaming sound is validated before window/device setup. Unmuted movies
with root streams report unsupported playback instead of silently omitting audio.
With --mute the player uses the additive [fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L90) API and
[FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../include/flashvm/movieclip.h#L82). It validates and omits only root stream controls
from scene playback. The original [fvm_movieclip_load_swf](../include/flashvm/movieclip.h#L43) remains strict.

The opt-in requires its decoded input to match the current [fvm_load_memory](../include/flashvm/flashvm.h#L72) source
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../include/flashvm/movieclip.h#L43) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:90](../include/flashvm/movieclip.h#L90) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../include/flashvm/mouse.h#L50) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:82](../include/flashvm/movieclip.h#L82) |
<!-- source-links:end -->

# SWL 0.3.0 resources

Resources are added through C API or CLI. The linker allocates CharacterIds,
emits definitions before code and exports names using ExportAssets. JPEG uses
DefineBitsJPEG2 plus a separate bitmap-filled DefineShape3; the exported symbol
is displayable. Resource-only SWF files need no `.swo` input.

```sh
build/linker/swl link scene.swf \
  --svf panel linker/examples/panel.svf \
  --jpeg tile linker/examples/tile.jpg \
  --wav tone linker/examples/tone.wav \
  --place panel 20 30 1 --place tile 150 30 2 --play tone
```

| Input | C API | SWF tags |
| --- | --- | --- |
| SVF1 / rectangle | `swl_add_svf`, `swl_add_rect` | DefineShape3 |
| Baseline JPEG | `swl_add_jpeg` | DefineBitsJPEG2 + DefineShape3 |
| PCM WAV | `swl_add_wav` | DefineSound |
| Display placement | `swl_place(name,x,y,depth)` | PlaceObject2 |
| Sound start | `swl_play(name)` | StartSound |

The example contains a green 120×80 panel, an 8×8 blue image and a short 440 Hz
tone. Contexts copy inputs; failed resource additions leave them unchanged.
Writers are called only after complete assembly/validation. Resource names and
module names occupy separate namespaces. Placements require an already-added
resource and a unique depth 1..65535; x/y are nonnegative pixels 0..3276. Placement
and sound-start events appear in event order before DoAction in the single frame.

SVF1 is an **initial project-specific subset**, not SVG:
`SVF 1 rect WIDTH HEIGHT RRGGBBAA`, with ASCII whitespace-separated tokens.
Exactly one rectangle, dimensions 1..3276 pixels, eight hexadecimal RGBA digits.
No arbitrary paths, gradients, strokes or transforms. Future shapes require a
versioned contract. [Example](examples/panel.svf).

JPEG: baseline SOF0, 8-bit, 1/3 components, SOI/SOS/EOI, dimensions up to 3276.
Headers and segment bounds are validated; entropy data is not decoded here.
Progressive JPEG, PNG/GIF and entropy-code diagnostics are unsupported. WAV:
RIFF/WAVE PCM, mono/stereo, 8/16-bit little endian, 5512/11025/22050/44100 Hz;
chunks, block alignment and frame counts are validated. No compressed WAV/MP3.
JPEG/WAV are each limited to 16 MiB; total context limit 64 MiB, at most 1024
resources and 1024 events. Initial `.swa` libraries carry code `.swo` members;
resources are supplied separately.

Tests independently decode RECT, shape edges, bitmap bytes, PCM samples, exports,
placements and StartSound. The optional FlashVM runner loads the resulting SWF.
This verifies loading/structure, not physical display rendering or audio-device
playback.

# SWL example catalog

All examples use the public API or documented command interface. They do not
require implicit sibling implementation includes. [Linker guide](../README.md),
[build and cross-build guide](../../BUILDING.md).

| Example | Purpose and requirements |
| --- | --- |
| [IDE pipeline](ide/README.md) | Explicitly supplies four language parser SDKs, FIR and SWL to a C application; in-memory source/FIR/SWO/library/movie path, ownership and writer callbacks. Numeric subset only. |
| [Inspect](inspect/README.md) | Standalone C99 host, SWL SDK only; validates a SWO/SWA before reporting ordered module metadata. Detailed callback and input lifetimes. |

Resource fixtures [panel.svf](panel.svf), [tile.jpg](tile.jpg), [tone.wav](tone.wav)
are used by [resource instructions](../ASSETS.md) and format tests: one filled
rectangle, an 8×8 JPEG and PCM WAV. They are assets, not source compilers or full
SVG/MPEG support. See that guide for exact resource/placement/play commands.

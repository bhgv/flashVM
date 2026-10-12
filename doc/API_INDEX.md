# FlashVM C89/C++98 API index

- `include/flashvm/gc.h`: collection, mode, pinning, introspection.
- `include/flashvm/value.h`: primitive and object values.
- `include/flashvm/table.h`: indexed elements and metatables.
- `include/flashvm/channel.h`: raw channel transport.
- `include/flashvm/transfer.h`: graph send/receive across queue/file/TCP channels.
- `include/flashvm/property.h`: property flags, deletion and enumerable-name snapshots.
- `include/flashvm/function.h`: first-class native/AVM1 functions, invoke/construct.
- `include/flashvm/movieclip.h`: host-authored clip frames and vector rendering.
- `include/flashvm/standard.h`: standard installation and primitive conversion.
- `include/flashvm/capability.h`: TCP resource authority and imports.

All public headers must remain valid C89 and usable from C++98. No STL types cross the C ABI.

Object graph coverage and limitations: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

# FlashVM C89/C++98 API index

- `include/flashvm/gc.h`: collection, mode, pinning, introspection.
- `include/flashvm/value.h`: primitive and object values.
- `include/flashvm/object.h`: properties and native ownership.
- `include/flashvm/table.h`: indexed elements and metatables.
- `include/flashvm/channel.h`: typed channel transport (if available in this build).

All public headers must remain valid C89 and usable from C++98. No STL types cross the C ABI.

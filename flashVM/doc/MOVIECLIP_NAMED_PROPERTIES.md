# Named MovieClip dimensions

[Русский](MOVIECLIP_NAMED_PROPERTIES.ru.md) · [Images](IMAGE_LOADING.md) ·
[Sources](PLAYER_SOURCES.md) · [Stage status](ROADMAP_STATUS.md).

Since v0.704, `_width` and `_height` on a MovieClip instance read and resize the
same native painted geometry as numeric AVM1 properties 8 and 9. This includes
loaded image rectangles, drawings and child content. Translation and ancestor
scaling do not change the reported parent-space dimensions; the clip's own scale
and rotation do. Reading does not cache a scalar property that becomes stale
after drawing, loading or unloading.

```actionscript
// A sketch: run after the child image has completed loading.
trace(picture._width); // Parent-space painted extent, including its own transform.
picture._width = 160; // Change native scales using the existing AVM1 resize rule.
picture._height = 90; // The next read and render see the same geometry.
```

Numeric input uses the existing AVM1 conversion profile. A coercible object can
run user `valueOf`/`toString`; conversion errors propagate before scale publication.
Absent, null and undefined assignments are ignored. NaN follows the existing
intrinsic no-op rule. Negative, rotated, degenerate and extreme dimensions retain
the existing numeric-property behavior; this change does not introduce a new
geometry algorithm. Empty clips read zero. The class prototype reads undefined
and ignores assignments. Accessors are nonenumerable and nondeletable on the
builtin prototype; ordinary user property descriptors can still shadow them.
Numeric ActionGetProperty/ActionSetProperty continue to bypass named shadows.

The C host can read/write through [`fvm_object_get`](../include/flashvm/value.h#L72) / [`fvm_object_set`](../include/flashvm/value.h#L74) without a
console or frontend dependency. Their normal same-context validation, borrowed
result storage and error rules apply. Keep the context and receiver alive;
[`fvm_gc_pin`](../include/flashvm/gc.h#L48) is required when a host pointer must survive a later collecting call.
Accessor invocation retains operands during script conversion, and native bounds
snapshots prevent collection while geometry callbacks inspect descendants.
Explicit collecting reentry remains subject to the normal native-call restriction.

The four adapters have separate stable graph callback IDs, with no native pointer
userdata. Serialization/channel transport restores their functions in the receiving
context; changing the copy's dimension does not mutate the original geometry.
Existing method IDs and the public descriptor ABI are unchanged.

## Focused verification

`tests/movieclip_named_dimensions.cpp` covers SWF5–8 profiles in interpreter/JIT
mode: named and numeric agreement, actual GetMember/SetMember actions, parent
scale, rotation, empty clips, null/undefined/NaN, conversion failures, collection,
class prototype and independent serialized graphs. The existing quality and image
loading tests remain regression controls. A separate allocation sweep walks C++
new/new[] failures and requires both old scales and dimensions to survive each
failed named resize. This is bounded implementation evidence,
not a complete original Flash Player compatibility or physical-device result.

```sh
cmake -S flashVM -B build-named
cmake --build build-named --target flashvm_tests_movieclip_named_dimensions
ctest --test-dir build-named -R '^flashvm_tests_movieclip_named_dimensions$' --output-on-failure
```

The test runs without SDL, networking or image codecs. See the linked image
examples for the optional desktop image profile and native/cross build commands.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../include/flashvm/value.h#L72) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
<!-- source-links:end -->

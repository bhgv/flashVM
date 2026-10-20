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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../include/flashvm/mouse.h#L50) |
<!-- source-links:end -->

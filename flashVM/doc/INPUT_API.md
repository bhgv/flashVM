# Input API (C89)

`flashvm/input.h` defines [`FVM_InputRule`](../include/flashvm/input.h#L33), [`FVM_InputEvent`](../include/flashvm/input.h#L42), [`fvm_input_set_rule`](../include/flashvm/input.h#L52) and [`fvm_input_map`](../include/flashvm/input.h#L54). The host detects touches and swipes, passes gesture type, finger count and direction, and receives a mapped mouse button (left, right, middle, wheel up/down). Mapping is per context. Exact direction wins over wildcard. **There is no touchscreen recognizer or event injection into the Flash display list yet.** See `examples/embedding/input`.


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L69), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L71), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L73), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L103).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L78), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L80), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90).


[Explicit host keyboard focus / Явный выбор клипа](KEYBOARD_FOCUS.md) ([RU](KEYBOARD_FOCUS.ru.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L83), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L86).

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L101).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:78](../include/flashvm/keyboard.h#L78) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../include/flashvm/keyboard.h#L71) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `fvm_input_map` | function | [include/flashvm/input.h:54](../include/flashvm/input.h#L54) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:80](../include/flashvm/keyboard.h#L80) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:69](../include/flashvm/keyboard.h#L69) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `fvm_input_set_rule` | function | [include/flashvm/input.h:52](../include/flashvm/input.h#L52) |
| `FVM_InputEvent` | struct | [include/flashvm/input.h:42](../include/flashvm/input.h#L42) |
| `FVM_InputRule` | struct | [include/flashvm/input.h:33](../include/flashvm/input.h#L33) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:103](../include/flashvm/keyboard.h#L103) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
<!-- source-links:end -->

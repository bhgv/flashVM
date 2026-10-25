# INPUT_API — русская документация

Приложение-хост распознаёт касания и свайпы, а FlashVM сопоставляет их с действиями мыши. Для синхронного получения событий используйте [`fvm_input_set_callback`](../include/flashvm/input.h#L86) и [`fvm_input_dispatch`](../include/flashvm/input.h#L104). Подробности: `INPUT_EVENTS.md`. Автоматической передачи в `MovieClip.onPress` пока нет.

Подробные сигнатуры и ограничения см. в `INPUT_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L73), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L77), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L81), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L127).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L88), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L92), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L108).

[Выбор клипа хостом](KEYBOARD_FOCUS.ru.md) ([EN](KEYBOARD_FOCUS.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L97), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L102).

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L123).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:88](../include/flashvm/keyboard.h#L88) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:104](../include/flashvm/input.h#L104) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:108](../include/flashvm/keyboard.h#L108) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:77](../include/flashvm/keyboard.h#L77) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:102](../include/flashvm/keyboard.h#L102) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:92](../include/flashvm/keyboard.h#L92) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:81](../include/flashvm/keyboard.h#L81) |
| `fvm_input_set_callback` | function | [include/flashvm/input.h:86](../include/flashvm/input.h#L86) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:97](../include/flashvm/keyboard.h#L97) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:127](../include/flashvm/keyboard.h#L127) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:123](../include/flashvm/keyboard.h#L123) |
<!-- source-links:end -->

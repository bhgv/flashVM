# INPUT_API — русская документация

Приложение-хост распознаёт касания и свайпы, а FlashVM сопоставляет их с действиями мыши. Для синхронного получения событий используйте [`fvm_input_set_callback`](../include/flashvm/input.h#L69) и [`fvm_input_dispatch`](../include/flashvm/input.h#L82). Подробности: `INPUT_EVENTS.md`. Автоматической передачи в `MovieClip.onPress` пока нет.

Подробные сигнатуры и ограничения см. в `INPUT_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L60), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L62), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L64), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L75).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L66), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L68), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L72).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:66](../include/flashvm/keyboard.h#L66) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:72](../include/flashvm/keyboard.h#L72) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:62](../include/flashvm/keyboard.h#L62) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:68](../include/flashvm/keyboard.h#L68) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:64](../include/flashvm/keyboard.h#L64) |
| `fvm_input_set_callback` | function | [include/flashvm/input.h:69](../include/flashvm/input.h#L69) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:60](../include/flashvm/keyboard.h#L60) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:75](../include/flashvm/keyboard.h#L75) |
<!-- source-links:end -->

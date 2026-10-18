# INPUT_API — русская документация

Приложение-хост распознаёт касания и свайпы, а FlashVM сопоставляет их с действиями мыши. Для синхронного получения событий используйте [`fvm_input_set_callback`](../include/flashvm/input.h#L69) и [`fvm_input_dispatch`](../include/flashvm/input.h#L82). Подробности: `INPUT_EVENTS.md`. Автоматической передачи в `MovieClip.onPress` пока нет.

Подробные сигнатуры и ограничения см. в `INPUT_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L59), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L61), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L63), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L65).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:61](../include/flashvm/keyboard.h#L61) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:63](../include/flashvm/keyboard.h#L63) |
| `fvm_input_set_callback` | function | [include/flashvm/input.h:69](../include/flashvm/input.h#L69) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:59](../include/flashvm/keyboard.h#L59) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:65](../include/flashvm/keyboard.h#L65) |
<!-- source-links:end -->

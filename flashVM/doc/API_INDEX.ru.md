# Указатель C API FlashVM

- `gc.h`: сборка мусора, режимы, закрепление объектов и диагностика.
- `value.h`: значения примитивов и объектов.
- `value.h`: также свойства, регистрация функций и время жизни объектов.
- `avm1.h`: действия AVM1, обработчики Trace/URL и просмотр активного target.
- `stage.h`: получение, задание и сброс RGBA-фона.
- `movieclip.h`: импорт SWF, кадры клипа, матрицы и отрисовка.
- `table.h`: числовые элементы и метатаблицы.
- `channel.h`: передача типизированных сообщений, если модуль включён.

Все публичные заголовки должны компилироваться как C89 и C++98. STL-типы не должны пересекать границу C ABI.

[`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L33) — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.


Prepared PCM stream snapshots / Независимые индексированные снимки PCM: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm). Six C89 functions copy/index one validated root PCM segment; indexed reads survive source VM destruction without allocation. Preparation is explicitly budgeted; compressed codecs and automatic MovieClip audio clocks remain unsupported.


Windows x64 backend selection, W^X and OS unwind-table ownership / выбор Windows x64 backend, W^X и время жизни unwind: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). Native Windows acceptance is pending / нативная приёмка Windows ещё не выполнена.


macOS Intel classic JIT memory policy, frame chain and host preparation / память JIT, цепочка кадров и подготовка host: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). Apple Silicon/MAP_JIT and native macOS acceptance remain open / Apple Silicon/MAP_JIT и нативная приёмка остаются открытыми.


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L61), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L63), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L65), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L88).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L70), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L72), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L82).

[Выбор клипа хостом](KEYBOARD_FOCUS.ru.md) ([EN](KEYBOARD_FOCUS.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L75), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L78).

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L86).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:70](../include/flashvm/keyboard.h#L70) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:82](../include/flashvm/keyboard.h#L82) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:63](../include/flashvm/keyboard.h#L63) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:78](../include/flashvm/keyboard.h#L78) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:72](../include/flashvm/keyboard.h#L72) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:65](../include/flashvm/keyboard.h#L65) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:61](../include/flashvm/keyboard.h#L61) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:75](../include/flashvm/keyboard.h#L75) |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:33](../include/flashvm/movieclip.h#L33) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:88](../include/flashvm/keyboard.h#L88) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
<!-- source-links:end -->

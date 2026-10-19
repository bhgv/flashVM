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


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L69), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L71), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L73), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L103).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L78), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L80), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90).

[Выбор клипа хостом](KEYBOARD_FOCUS.ru.md) ([EN](KEYBOARD_FOCUS.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L83), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L86).

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L101).


KeyPress v0.628: [`fvm_input_clip_key_code`](../include/flashvm/keyboard.h#L66) проверяет копию transition и переводит
special/printable selectors без allocation/мутации. Отдельный
[`fvm_movieclip_dispatch_key_press`](../include/flashvm/clip_actions.h#L97) рассылает точные retained selectors.
[Контракт](CLIP_KEYPRESS.ru.md) / [EN](CLIP_KEYPRESS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:78](../include/flashvm/keyboard.h#L78) |
| `fvm_input_clip_key_code` | function | [include/flashvm/keyboard.h:66](../include/flashvm/keyboard.h#L66) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../include/flashvm/keyboard.h#L71) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:80](../include/flashvm/keyboard.h#L80) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:69](../include/flashvm/keyboard.h#L69) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `fvm_movieclip_dispatch_key_press` | function | [include/flashvm/clip_actions.h:97](../include/flashvm/clip_actions.h#L97) |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:33](../include/flashvm/movieclip.h#L33) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:103](../include/flashvm/keyboard.h#L103) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
<!-- source-links:end -->

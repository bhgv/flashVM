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

[`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L25) — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.


Prepared PCM stream snapshots / Независимые индексированные снимки PCM: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm). Six C89 functions copy/index one validated root PCM segment; indexed reads survive source VM destruction without allocation. Preparation is explicitly budgeted; compressed codecs and automatic MovieClip audio clocks remain unsupported.


Windows x64 backend selection, W^X and OS unwind-table ownership / выбор Windows x64 backend, W^X и время жизни unwind: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). Native Windows acceptance is pending / нативная приёмка Windows ещё не выполнена.


macOS Intel classic JIT memory policy, frame chain and host preparation / память JIT, цепочка кадров и подготовка host: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). Apple Silicon/MAP_JIT and native macOS acceptance remain open / Apple Silicon/MAP_JIT и нативная приёмка остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:25](../include/flashvm/movieclip.h#L25) |
<!-- source-links:end -->

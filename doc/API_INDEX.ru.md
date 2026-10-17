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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:25](../include/flashvm/movieclip.h#L25) |
<!-- source-links:end -->

# Служебные теги SWF / SWF document tags — v0.121

FileAttributes (69) принимается первым тегом с ровно четырьмя байтами:
поддержаны нулевые флаги и HasMetadata (0x10). Metadata (77) принимается один раз
при установленном HasMetadata; это ровно одна завершённая NUL строка корректного
скалярного UTF-8. Объявленные метаданные должны присутствовать. XML не разбирается
и не выполняется. Эти нейтральные контейнерные теги принимаются и у старых
заголовков SWF, включая исходные SWF 6, произведённые тестовым компилятором;
заголовки не меняются и теги не удаляются для сравнения.

UseNetwork, AVM2 и аппаратные флаги возвращают UNSUPPORTED; зарезервированные
биты, дубли, неверные размеры/порядок и повреждённый UTF-8 — INVALID_ARGUMENT.
Это ограниченная реализация, а не завершённая поддержка всех FileAttributes
или sandbox. Правила битов сопоставлены с
[pinned primary types](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/swf/src/types.rs).

Атрибуты и строка сохраняются в приватном графе кадров MovieClip и передаются
в другие контексты. C89 API [`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L41) читает данные без
пользовательских getter и сохраняет выход при ошибке. `struct_size` обязателен;
metadata — undefined при отсутствии, counted string при наличии. Для чтения
после GC нужен корень клипа или удержанное значение. Замена загруженных байтов
контекста не инвалидирует строку, принадлежащую графу клипа.

Тесты: swf_document, swf_document_allocation, swf_document_api и swf_background.
Проверяются перенос/GC, пустая строка, C89 интерфейс, отрицательные случаи и
каждый отказ выделения памяти до первого успешного выполнения. Новые внешние
пробы ещё не считаются прошедшими: они также выявляют ошибки Array, InitArray,
Object и отсутствие подтверждения host FSCommand. Все стадии роудмапа открыты.

## English

Root FileAttributes must be first and exactly four bytes. Zero flags and
HasMetadata are supported; one declared Metadata tag must contain exactly one
terminated scalar UTF-8 string. XML is neither parsed nor executed. Neutral
container tags are accepted in original older-header fixtures too, without
rewriting headers or removing tags.

Network sandbox, AVM2 and hardware flags remain explicitly unsupported; reserved
bits and malformed/duplicate/misordered data are invalid. This is a limited
subset, with bit definitions checked against the pinned primary source above.

Document flags and owned metadata travel in the private MovieClip frame graph.
The new C89 query reads without script getters and leaves output unchanged on
errors. A clip root or retained value is needed across GC; replacing the context's
loaded SWF bytes does not replace graph-owned metadata. Tests above cover graph
transport, real collection, empty payloads, C89, malformed inputs and exhaustive
allocation-failure retries. Newly retrieved oracle probes still expose other
Array/Object/InitArray and host-command gaps. No roadmap stage is closed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:41](../include/flashvm/movieclip.h#L41) |
<!-- source-links:end -->

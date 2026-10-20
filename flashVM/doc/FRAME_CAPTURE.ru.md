# RGBA-кадры плеера для C-хоста

[English](FRAME_CAPTURE.md) · [Управление](PLAYBACK_CONTROL.ru.md) ·
[Потоки контроллера](PLAYBACK_MAILBOX.ru.md) · [План](ROADMAP_STATUS.md).

## Контракт и копирование с v0.716

[FVM_DesktopFrame](../frontend/common/frame.h#L18) описывает заимствованное owner-thread наблюдение native-растра:
RGBA8, width/height, stride, byte_count, pixels/source и счётчики тактов/показов.
[FVM_DESKTOP_FRAME_RGBA8](../frontend/common/frame.h#L5) означает байты R,G,B,A с прямой альфой, слева направо,
строки сверху вниз; это не endian-зависимое целое. Видимая строка содержит
width * [FVM_DESKTOP_FRAME_PIXEL_BYTES](../frontend/common/frame.h#L7) байт. Размеры ограничены
[FVM_DESKTOP_FRAME_MAX_DIMENSION](../frontend/common/frame.h#L8), то есть текущим пределом stage/raster.

Это логическая сцена, а не screenshot окна: window chrome, DPI/letterboxing и
повторные native swaps при resize в неё не входят. Причины наблюдения —
[FVM_DESKTOP_FRAME_TICK](../frontend/common/frame.h#L10) после host advance и [FVM_DESKTOP_FRAME_REFRESH](../frontend/common/frame.h#L11) для
начального/dirty redraw без host tick. Счётчики презентаций начинаются с одного;
movie_ticks/presentations относятся к текущему корню, total_* — ко всему запуску.

[FVM_DesktopFrameNotify](../frontend/common/frame.h#L44)(frame, userdata)` — callback владельца после успешного
растра и native-показа, вне рендеринга VM. Возвращает [FVM_OK](../include/flashvm/flashvm.h#L42) для продолжения;
другая ошибка завершает playback после уже совершённых действий/рисования.
Заимствованные frame/pixels/source действительны только до возврата. Не меняйте
pixels, не сохраняйте указатели, не входите в VM/плеер повторно, не завершайте SDK,
не забирайте SDL events и не пропускайте C++-исключения через C. Для другого потока
сначала скопируйте пиксели в свою память. [GC:] Здесь нет управляемого владельца
или pin: сохранение frame не сохраняет ни native buffer, ни прежнюю VM.

[FVM_DesktopFrameObserver](../frontend/common/frame.h#L47) — отдельный callback-дескриптор; прежние раскладки
source/options/loader/control не меняются. [fvm_desktop_frame_observer_init](../frontend/common/frame.h#L57)(out)`
заполняет точный struct_size и нулевые notify/userdata, возвращает 1/0 для обычного/
null указателя, без выделения памяти и старта SDK. Для наблюдаемого запуска notify
обязателен. Сам дескриптор и userdata живут до синхронного возврата runner.

[fvm_desktop_frame_copy](../frontend/common/frame.h#L65)(frame, destination, capacity, destination_stride)` копирует
только видимые RGBA-байты строк. Память не выделяется, VM/SDL не запускаются.
При [FVM_OK](../include/flashvm/flashvm.h#L42) источник не изменён, padding назначения остаётся прежним. Для ошибки
возвращается [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) и никакие байты назначения не меняются.
Проверяются указатели, точный struct_size, format, ненулевые ограниченные размеры,
оба stride и обе доступные длины, включая переполнение (height-1)*stride+row_bytes.
Последний padding источника не требуется: достаточно полного видимого последнего
ряда. Метаданные/source и padding не копируются. Для единственной строки очень
большой stride сам по себе не ошибка, если видимая строка доступна.

Назначение должно быть доступно и не пересекаться с источником: это предусловие
владения вызывающего, не проверка принадлежности произвольных C-указателей.
Не передавайте уже освобождённые/невалидные указатели. Для ошибки capacity/geometry
сначала проверяется весь диапазон и лишь затем начинается копирование.

## Проверки и границы

`frontend/tests/frame_copy.c` использует независимый raw-byte oracle: разные stride,
сохранение padding, усечённый источник/назначение, неизвестный format, переполнение
и неизменность памяти при ошибке. Тест C89 не запускает VM/SDL. v0.716 вводит
контракт и безопасное копирование; runner-интеграция описывается при добавлении.
Это не windowless widget, не новое управление GPU и не завершение стадий плана.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->

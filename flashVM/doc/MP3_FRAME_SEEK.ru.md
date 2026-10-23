# Покадровая перемотка подготовленного MP3

Подготовка MP3 сохраняет исходный декодированный префикс и знаковое SeekSamples
каждого блока перед освобождением MPEG-байтов. Два полных прохода сохраняют
резервуар декодера; итоговый PCM обрезает начальную задержку один раз.
Неизменяемый нативный владелец переживает уничтожение VM.

Для последующего авторского блока R — исходный декодированный префикс,
S — знаковое SeekSamples. Спецификация Adobe SWF (страницы 185–187) прибавляет
начальную задержку L при перемотке: исходная позиция R+L+S. В уже обрезанном
PCM это R+S. Первый блок начинает с нормализованного нуля. Непрерывное
воспроизведение не меняется. [Спецификация Adobe](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

Отрицательная начальная задержка, полное эталонное планирование A/V, чтение
вперёд, дочерние потоки и приёмка на физических устройствах остаются открытыми.
Все девять стадий плана открыты. Версия не подтверждает полноту SWF 8.

## Запрос неизменяемой позиции

Установите точный struct_size структуры [FVM_SWFStreamSoundSeekPoint](../include/flashvm/swf_stream_sound.h#L142) и вызовите
[fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L161) с положительным номером кадра от единицы.
Первый авторский блок не раньше запроса определяет source_frame. block_index и
first выбирают нормализованный PCM, возможно из другого блока. PCM/ADPCM
выбирают нулевое смещение. Отсутствие блока даёт EOF с
[FVM_SWF_STREAM_SEEK_EOF_FRAME](../include/flashvm/swf_stream_sound.h#L140); точный конец PCM сохраняет авторский кадр.
Отрицательная/выходящая за конец позиция, неверные указатели/размер и нулевой
кадр дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); закодированный сжатый источник — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46);
переполнение — [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44). При ошибке выход не меняется. Нет выделения,
декодирования, callback, скрипта или доступа к GC. Параллельные const-запросы
после уничтожения VM допустимы; закрытие синхронизирует владелец. Разрешение
кадров, сброс фазы ресемплера и транзакция очереди остаются обязанностью хоста.


## Транзакция вывода

[fvm_swf_stream_output_bind_frame_aligned](../include/flashvm/swf_stream_output.h#L176) проверяет геометрию и позицию до callback;
[fvm_swf_stream_output_seek_frame_aligned](../include/flashvm/swf_stream_output.h#L179) использует текущий конвертер. Оба
используют pause → clear → публикация. Только успешный clear публикует курсор/
источник и сбрасывает ожидающий PCM, счётчики и фазу ресемплера. Ошибка clear
сохраняет прежние PCM/курсоры, но подтверждённая пауза остаётся видимой.
Статусы callback передаются, исключения дают [FVM_ERROR](../include/flashvm/flashvm.h#L43), повторный вход отклонён.
Нет выделения/декодирования/VM/GC. Владелец сохраняет snapshot, converter,
backend и output и сериализует изменения. Успех оставляет паузу. Прежние
bind_frame/seek_frame продолжают выбирать начало авторского блока.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_swf_stream_output_bind_frame_aligned` | function | [include/flashvm/swf_stream_output.h:176](../include/flashvm/swf_stream_output.h#L176) |
| `fvm_swf_stream_output_seek_frame_aligned` | function | [include/flashvm/swf_stream_output.h:179](../include/flashvm/swf_stream_output.h#L179) |
| `FVM_SWF_STREAM_SEEK_EOF_FRAME` | macro / define | [include/flashvm/swf_stream_sound.h:140](../include/flashvm/swf_stream_sound.h#L140) |
| `fvm_swf_stream_sound_snapshot_seek_point` | function | [include/flashvm/swf_stream_sound.h:161](../include/flashvm/swf_stream_sound.h#L161) |
| `FVM_SWFStreamSoundSeekPoint` | struct | [include/flashvm/swf_stream_sound.h:142](../include/flashvm/swf_stream_sound.h#L142) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->

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

Установите точный struct_size структуры [FVM_SWFStreamSoundSeekPoint](../include/flashvm/swf_stream_sound.h#L143) и вызовите
[fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L162) с положительным номером кадра от единицы.
Первый авторский блок не раньше запроса определяет source_frame. block_index и
first выбирают нормализованный PCM, возможно из другого блока. PCM/ADPCM
выбирают нулевое смещение. Отсутствие блока даёт EOF с
[FVM_SWF_STREAM_SEEK_EOF_FRAME](../include/flashvm/swf_stream_sound.h#L141); точный конец PCM сохраняет авторский кадр.
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


## Интеграция корневого desktop-потока

Непоследовательная навигация корня использует aligned binding под аудиомьютексом.
Владелец VM публикует только скаляры. Выбранный будущий авторский блок не звучит
в предшествующем пустом кадре даже при отрицательном смещении. После разрешения
авторского кадра доступен его начальный PCM-блок, даже если положительное
смещение/обрезка задержки перенесло его дальше. Продвигается только начальный
блок; последующие сохраняют прежний gate. Авторский Stop, пауза хоста и незавершённые
callback кадра продолжают удерживать вывод. Непрерывный путь не применяет
SeekSamples повторно. Эталонное чтение вперёд, source-time и коррекция A/V открыты.


## API и границы выполнения

| Операция | Результат и владение |
| --- | --- |
| [fvm_swf_stream_sound_snapshot_seek_point](../include/flashvm/swf_stream_sound.h#L162) | Копирует позицию, не меняет snapshot и выход при ошибке; точный размер и срок жизни задаёт владелец. |
| [fvm_swf_stream_output_bind_frame_aligned](../include/flashvm/swf_stream_output.h#L176) | Проверяет другой converter, вызывает pause/clear, затем публикует привязку; оба converter принадлежат хосту. |
| [fvm_swf_stream_output_seek_frame_aligned](../include/flashvm/swf_stream_output.h#L179) | Та же транзакция для текущего converter. |
| [fvm_swf_stream_pcm_reader_seek](../include/flashvm/swf_stream_pcm.h#L54) | Применяет block_index/first к offline reader без воздействия на очередь. |
| [fvm_swf_stream_converter_seek](../include/flashvm/swf_stream_convert.h#L73) | Устанавливает source-позицию и нулевую рациональную фазу; синхронизация у хоста. |

Смещения считаются в sample frames/парах, не channel values и не выходных кадрах.
Поиск логарифмический по индексированным блокам. Весь PCM и metadata находятся
в bounded-владельце; sizeof приватного индекса входит в runtime и peak-бюджет.
Прежние публичные структуры не увеличены. Скопированная позиция не владеет
snapshot: она применима только к исходному сегменту. Нельзя подставлять её к
другому источнику или закрывать snapshot до завершения reader/worker.

Выход SeekSamples за PCM отклоняется без clamp и выдуманной тишины. Callback
backend обязан сохранять all-or-none контракт при ошибке. Отклонённые записи
сохраняют точный PCM; успешная перемотка отменяет его после подтверждения clear,
в том числе через mixed-адаптер. Это позиционирование в конечном подготовленном
буфере, а не restart декодера в произвольно обрезанном MPEG и не полная Flash-приёмка.

## Воспроизводимые проверки

Соберите `flashvm_tests_swf_stream_mp3_seek_point`,
`flashvm_tests_swf_stream_mp3_seek_output`,
`flashvm_tests_swf_stream_mp3_seek_allocation` и
`flashvm_tests_swf_stream_mp3_seek_threads`. С явным SDL2 SDK и desktop frontend
также соберите `flashvm_frontend_stream_mp3_seek`. C89-пример
`flashvm_examples_multimedia_swf_stream_mp3_main --seek INPUT FRAME OUTPUT`
выгружает хвост после уничтожения VM. `scripts/check_mp3_seek_example.py EXECUTABLE`
сравнивает полные хвосты с FFmpeg; отсутствие FFmpeg/ffprobe явно даёт skip 77.
Нативные/dummy/аналитические проверки отделены от физических устройств.


## Проверенный объём

Авторская MP3-перемотка v0.1077–v0.1089: snapshot сохраняет исходные декодированные префиксы и знаковые смещения; новые immutable position и transactional output API применяют их только при навигации. Desktop сохраняет разрешение выбранного авторского кадра, включая обратные позиции через пустой кадр и начальный блок, сдвинутый задержкой. Прежние block-start API и непрерывный PCM сохраняют поведение. Прошла 241 выбранная native regression/tutorial/CLI проверка, пять full-profile ASan/UBSan и отдельные no-throw output/no-fault allocation запуски, девять SDK-free проверок. Исключения callback и четыре allocation failure проверены обычной сборкой; LeakSanitizer отключён. Шесть MPEG-векторов проверяют знаковые границы, две задержки, пустые блоки, точный конец, неизменность выхода при ошибке и обычный PCM. Четыре OS-worker выполняют 256 post-VM эпох; без выделений проверены 128 эпох. Независимый FFmpeg сравнивает 192 полных C89-выгрузки для FWS/CWS, обоих знаков, задержки и EOF. Настоящие SDL dummy-очереди проверяют MovieClip interpreter/native-JIT и GC; работающий worker не отправляет PCM до выбранного будущего авторского кадра и продолжает после разрешения. Переименованный отделённый SDK-free компонент конфигурируется, скопированный oracle проходит на native exporter; отдельной detached-сборки не было. Отрицательная начальная задержка, произвольный MPEG restart, полное эталонное чтение вперёд/source-time A/V drift, дочерние таймлайны/словари, устройства/другие платформы и все девять стадий остаются открытыми. [Контракт](MP3_FRAME_SEEK.ru.md).

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
| `fvm_swf_stream_converter_seek` | function | [include/flashvm/swf_stream_convert.h:73](../include/flashvm/swf_stream_convert.h#L73) |
| `fvm_swf_stream_output_bind_frame_aligned` | function | [include/flashvm/swf_stream_output.h:176](../include/flashvm/swf_stream_output.h#L176) |
| `fvm_swf_stream_output_seek_frame_aligned` | function | [include/flashvm/swf_stream_output.h:179](../include/flashvm/swf_stream_output.h#L179) |
| `fvm_swf_stream_pcm_reader_seek` | function | [include/flashvm/swf_stream_pcm.h:54](../include/flashvm/swf_stream_pcm.h#L54) |
| `FVM_SWF_STREAM_SEEK_EOF_FRAME` | macro / define | [include/flashvm/swf_stream_sound.h:141](../include/flashvm/swf_stream_sound.h#L141) |
| `fvm_swf_stream_sound_snapshot_seek_point` | function | [include/flashvm/swf_stream_sound.h:162](../include/flashvm/swf_stream_sound.h#L162) |
| `FVM_SWFStreamSoundSeekPoint` | struct | [include/flashvm/swf_stream_sound.h:143](../include/flashvm/swf_stream_sound.h#L143) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->

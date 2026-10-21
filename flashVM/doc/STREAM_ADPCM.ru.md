# Подготовленные потоки ADPCM из целых пакетов

Явные достоверные длины неполных блоков: [counted ADPCM](STREAM_ADPCM_COUNTED.ru.md). Автоматический путь ниже сохраняет распознавание только целых пакетов.

[English](STREAM_ADPCM.md) · [снимки PCM](SWF_STREAM_SOUND.md) · [плеер](PLAYER_PCM_STREAMS.ru.md)

[`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L81) — дополнительная C89-функция подготовки.
Вызывается владельцем контекста до публикации данных worker. Возвращённый
неизменяемый снимок принадлежит вызывающему; освобождение — через
[`fvm_swf_stream_sound_snapshot_close`](../include/flashvm/swf_stream_sound.h#L131). После уничтожения контекста/runtime
reader/converter/output используют снимок без VM, аллокаций, декодирования
и обратных вызовов. Подготовка не продвигает MovieClip и не открывает устройства.

PCM_NATIVE/PCM_LE копируется прежним путём без изменения. Для ADPCM поддерживаются
целые пакеты по 4096 сэмплов, коды шириной 2–5 бит, моно/стерео. В каждом
SoundStreamBlock собственные ширина кодов, predictor/index. Двухбитный префикс
ширины предшествует всем пакетам блока. В конце допустимы не более семи нулевых
битов выравнивания. Пустой блок либо один байт ширины с нулевым заполнением
дают пустой marker. Блок может содержать несколько целых пакетов. Среднее число
сэмплов в заголовке не используется как фактическая длина.

Модель подразделения пакетов описана в разделах streaming sound и ADPCM
[спецификации Adobe SWF](https://open-flash.github.io/mirrors/swf-spec-19.pdf#page=185).
У короткого пакета внутри SoundStreamBlock нет явной длины в сэмплах. Поэтому
этот путь возвращает UNSUPPORTED, не угадывая размер последнего пакета.
Это явное ограничение реализации, а не утверждение, что все исторические файлы
Flash состоят исключительно из полных пакетов.

После успешной подготовки ADPCM представлен как PCM_NATIVE/S16 при исходной
частоте и числе каналов. Индексы потока/блока, номера кадров, рекомендованные
параметры и среднее сохраняются. `sample_count_known` устанавливается;
размеры и количество сэмплов описывают декодированные данные.
[`fvm_swf_stream_sound_snapshot_info`](../include/flashvm/swf_stream_sound.h#L121) и [`fvm_swf_stream_sound_snapshot_block_at`](../include/flashvm/swf_stream_sound.h#L125)
возвращают **нормализованное** представление. Исходные live-функции и
[`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65) продолжают выдавать оригинальные непрозрачные
ADPCM-байты. Для сохранения сжатого оригинала используйте raw API. Публичные
структуры ABI не меняются; существующие reader/converter работают с обычным PCM.

Аргументы: контекст-владелец, индекс потока с нуля, обязательный указатель результата.
Результат сначала обнуляется. Ошибки контекста/индекса передаются из raw-пути.
Другие кодеки и короткие пакеты дают UNSUPPORTED; ненулевые биты заполнения —
INVALID_ARGUMENT. Ошибка выделения либо превышение бюджета — OUT_OF_MEMORY.
Сжатый payload, раскрытый S16 и точный индекс блоков вместе должны укладываться
в memory-limit runtime либо прежний SWF-бюджет по умолчанию. Это ограничение
одной подготовки, не суммарного RSS всех живых снимков. Сжатое хранилище
освобождается до успешного возврата. При ошибке временные данные освобождаются,
частичный снимок не публикуется. Нельзя изменять/закрывать снимок во время чтения.

Существующий [учебный C89-пример](../examples/multimedia/swf_stream/README.md)
использует новую функцию в подготовленных режимах с прежним PCM-fixture.
Проверка сжатых данных после сборки `flashvm_tests_swf_stream_adpcm`:

```sh
./build/flashvm_tests_swf_stream_adpcm
python3 scripts/check_stream_adpcm_ffmpeg.py build/flashvm_tests_swf_stream_adpcm
```

FFmpeg нужен только как необязимый независимый тестовый оракул. Python независимо
упаковывает коды в одно-/двухпакетные блоки, добавляет пустые маркеры и неверное
среднее, затем сравнивает каждый сэмпл восьми сочетаний ширины/каналов для FWS/CWS.
Экспортёр уничтожает исходную VM до чтения снимка. C++-fixture дополнительно
проверяет сохранение raw-байтов, бюджет, ненулевое заполнение, отказ коротким
пакетам, перезапуск predictor и границы чанков. Это не приёмка физических устройств,
семантики Flash drift/seek, MP3/Nellymoser или исполнения на другом CPU.
Проверки ревизии: прошли 20 нативных core stream/ADPCM-тестов. Изменённые потоковые
модули и два новых fixture прошли ASan/UBSan; бюджеты выделения 0–4 проверялись
в отдельных процессах. LeakSanitizer отключён: перечисление процессов здесь
недоступно. Вся VM и внешние зависимости под sanitizer не пересобирались.
Общий проверщик комментариев мультимедиа продолжает находить прежние пропуски
в неизменённых файлах Windows speech; успешным этот общий прогон не объявляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_snapshot_block_at` | function | [include/flashvm/swf_stream_sound.h:125](../include/flashvm/swf_stream_sound.h#L125) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:131](../include/flashvm/swf_stream_sound.h#L131) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:121](../include/flashvm/swf_stream_sound.h#L121) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:81](../include/flashvm/swf_stream_sound.h#L81) |
<!-- source-links:end -->

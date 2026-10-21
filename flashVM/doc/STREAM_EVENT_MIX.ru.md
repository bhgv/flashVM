# Добавление событийных голосов к потоковому PCM (v0.771)

[English](STREAM_EVENT_MIX.md) · [Роудмап](ROADMAP.md) · [C89-пример](../examples/multimedia/swf_stream_mix/README.md)

С v0.772 [нативный адаптер](MIX_QUEUE.ru.md) удерживает исходный/смешанный
блок для точного retry и подтверждённого clear. Пример проверяет прямой экспорт
и настоящий ограниченный FIFO; desktop timeline integration остаётся открытой.

`fvm_swf_sound_mixer_add(mixer, frames, pcm, capacity_values, written_values)`
добавляет подготовленные event-голоса **на месте** к инициализированному S16 PCM.
Базой может быть подготовленный SWF stream, внешний нативный источник или явно
заданная хостом тишина. Частота/каналы базы соответствуют конфигурации mixer;
автоматического определения частоты и планирования таймлайна нет. Это первая
нативная композиция в разделе завершения функциональности; desktop mixed playback
пока не завершён.

База и все голоса попадают в общий ограниченный широкий аккумулятор; насыщение
выполняется один раз в конце прохода. База -30000 и два голоса +30000 дают +30000.
Если сначала обрезать сумму голосов до S16, затем прибавить базу, получится неверное
+2767. Без активных голосов база сохраняется. Прежний [fvm_swf_sound_mixer_read](../include/flashvm/swf_sound_mixer.h#L105)
сохраняет нулевую базу и тишину после окончания голосов. Оба входа используют
одни часы голосов и могут чередоваться при внешней сериализации. Конверсия,
циклы, envelope и stop сохраняют прежний контракт mixer/voice.

## Контракт C API

| Аргумент | Требование |
| --- | --- |
| mixer | Живой нативный mixer, все операции сериализованы хостом |
| frames | Число interleaved кадров, не отдельных значений каналов |
| pcm | Инициализированный выровненный S16 буфер владельца; перезаписывается при успехе |
| capacity_values | Доступное число значений, не меньше frames × channels |
| written_values | Ненулевой отдельный size_t; обнуляется до проверки |

OK записывает ровно frames × channels значений. Нулевой mixer, отсутствие
буфера при положительном frames, недостаточная ёмкость или переполнение диапазона
дают INVALID_ARGUMENT до изменения PCM/позиций голосов. Нулевой written_values
также INVALID_ARGUMENT. Нулевой frames разрешает null PCM и ничего не продвигает.
PCM не перекрывается с written_values или внутренним состоянием mixer. Нулевая
инициализация нужна для явно выбранной нулевой базы, а не для готовых stream-отсчётов.

Вызов не выделяет память, не вызывает callbacks, VM, managed-объекты и устройство.
Завершённые голоса могут освобождаться. Подготовка и submit выполняются на владельце
до публикации native-состояния; источник context/runtime затем можно уничтожить.
Хост удерживает mixer и PCM до завершения операции; mixer_close освобождает владение
после всех читателей. Внутреннего mutex/worker/скрытой очереди нет. 64 голоса плюс
одна S16-база укладываются в прежний signed-long accumulator на поддержанных хостах.

## Retry, хвосты и отмена

Успешный add продвигает голоса **один раз**, даже если следующая запись хоста
не удалась. Повторно посылать сохранённые смешанные байты; повторный add смешает
снова и продвинет голоса. Это не идемпотентный enqueue. Отката внешней записи и
позиции голосов API не делает. Backend adapter должен владеть pending mixed block
до принятия/отмены и согласовывать submit с его временем жизни. Частичный fwrite
считается терминальной ошибкой, а не атомарным retry очереди.

После stream EOF хост может явно подать нули для хвоста event-голосов. Длительность,
авторизацию и часы задаёт хост. Пропуск кадров фильма не разрешает автоматически
синтезировать stream или двигать общее аудиовремя. Для stop/seek/queued audio/tails
нужна согласованная политика. Desktop player по-прежнему отклоняет root stream+event
до полной интеграции. MP3, partial ADPCM, дочерние потоки и A/V drift не добавлены.

## Исполняемый пример

[C89-пример](../examples/multimedia/swf_stream_mix/README.md) содержит один настоящий
SWF8: два DefineSound/StartSound и root PCM stream. В его единственном кадре оба
события запускаются явно. После подготовки context/runtime уничтожаются. Нативные
converter и mixer экспортируют 16 stereo кадров 48000 Hz в четырёхкадровых буферах,
с хвостом более длинного event поверх явной нулевой базы. В цикле нет SDL/VM.
Python oracle независимо вычисляет абсолютные рациональные позиции, все отсчёты,
насыщение, хвост/тишину и проверяет replacement/CLI/native errors.

```sh
cmake --build build --target flashvm_tests_swf_sound_mixer_add \
  flashvm_tests_swf_sound_mixer_add_allocation flashvm_examples_multimedia_swf_stream_mix_main
ctest --test-dir build --output-on-failure -R 'swf_sound_mixer_add|swf_stream_mix'
python3 scripts/check_swf_stream_mix_example.py build/flashvm_examples_multimedia_swf_stream_mix_main
```

API-тест проверяет четыре SWF-частоты, mono/stereo, strict/converted, вызовы на 513
кадров через внутренние проходы 256 кадров, обе полярности, некорректные/огромные/
нулевые запросы и legacy read после EOF. Allocation fixture готовит 64 голоса,
уничтожает VM и запрещает выделения при add/retirement. C89 compile/link проверяет
новый символ. Полная Flash-совместимость и приёмка платформ/устройств открыты.

## Проверка ревизии

Пройдены 40 нативных sound/stream тестов, затем усиленная проверка чередования
read/add и точный C89 oracle. Изменённый mixer, два новых C++ fixtures и C89-пример
прошли ASan/UBSan; вся VM/зависимости не инструментированы, перечисление утечек
отключено на ограниченном хосте. Строгий C89 consumer слинкован и запущен через
shared player-host library с новым API.

Четыре выбранные программы (add, no-allocation, C89 ABI, tutorial) собраны и
слинкованы для ARM64/RISC-V64/x86-64 Linux/musl и Windows x64. Все 12 Linux образов
проверены на CPU/static ELF, четыре Windows — PE32+ x64/прямые OS imports. На
x86-64 хосте действительно запущены x86-64 musl tests и независимый точный oracle.
ARM64/RISC-V64/Windows и физический звук здесь не исполнялись. Общий comment
checker сохраняет прежние ошибки неизменённых Windows speech исходников;
новый пример проходит every-line comments и README/API coverage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mixer_add` | function | [include/flashvm/swf_sound_mixer.h:117](../include/flashvm/swf_sound_mixer.h#L117) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:105](../include/flashvm/swf_sound_mixer.h#L105) |
<!-- source-links:end -->

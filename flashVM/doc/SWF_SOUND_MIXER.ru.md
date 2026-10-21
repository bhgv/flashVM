# Порционный микшер событий StartSound

## Назначение и владение

C89 API [swf_sound_mixer.h](../include/flashvm/swf_sound_mixer.h) объединяет
независимые снимки [`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L9) в один native signed-16 PCM поток. В отличие
от [`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L142), голоса продолжают играть между чтениями и
смешиваются в одном выходном буфере. События явно передаёт хост. MovieClip и
системный аудиовыход автоматически этого не делают.
[Исполняемый пример](../examples/multimedia/swf_event_mixer/README.md) показывает
управление голосами и необязательную настоящую запись файла.

[GC:] Микшер владеет native слотами, PCM и огибающими; он не хранит context,
managed object или заимствованные исходные байты. Подготовленные голоса переживают
reload, GC, уничтожение context/runtime. Для submit нужен живой загруженный context
и его владеющий поток. Все операции одного микшера сериализуются хостом; это не
потокобезопасная очередь. Ни одна функция не открывает устройство, не создаёт поток
и не вызывает output callback. Нельзя закрывать микшер одновременно с чтением.

## Геометрия и пространство ID

Ниже описан прежний strict create. Явная configured-конвертация документирована
отдельно: [частота/каналы](SWF_SOUND_CONVERSION.ru.md). Старые функция создания
и info struct сохраняют ABI и строгую геометрию.

Частота фиксирована: 5512, 11025, 22050 или 44100 Гц; каналы — один или два,
interleaved. Ёмкость: 1..[`FVM_SWF_SOUND_MIXER_MAX_VOICES`](../include/flashvm/swf_sound_mixer.h#L9) (сейчас 64). Исходные
PCM8/PCM16/ADPCM преобразуются существующим декодером и подготовкой голоса.
Другие кодеки сохраняют прежние UNSUPPORTED/ошибки. Частота и число каналов должны
точно совпадать. Неявного resampling, mono/stereo conversion или master gain нет.

Sound ID общий для всего микшера. Используйте отдельный микшер для каждого
словаря SWF. Если намеренно подать другую VM с тем же ID, stop/no-multiple будут
действовать и на старые, и на новые голоса этого ID. Идентификатор context не
сохраняется. Другие микшеры и context audio channels — отдельные пространства
управления. AVM1 StopSounds не очищает независимый микшер: хост вызывает
[`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L89) и отдельно сбрасывает очередь устройства.

## Контракты функций

### fvm_swf_sound_mixer_create

`fvm_swf_sound_mixer_create(sample_rate, channels, voice_capacity, out)` создаёт
микшер и полную таблицу слотов. `*out` обнуляется до проверки. Null out, частота
вне перечисленных, channels вне 1..2, capacity вне 1..64 дают INVALID_ARGUMENT.
Несовместимые размеры byte/short дают UNSUPPORTED. Отказ new даёт OUT_OF_MEMORY,
handle не публикуется. После OK хост владеет handle до close. Context/устройство
не нужны. Фиксированные рабочие буферы не растут от длины запроса на чтение.

### fvm_swf_sound_mixer_submit

`fvm_swf_sound_mixer_submit(mixer, context, event_index)` разрешает корневой
StartSound по нулевому индексу через [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L62). Ошибки context,
незагруженной VM, повреждённых ресурсов и кодеков передаются из исходных API;
отсутствующий индекс даёт NOT_FOUND. На ошибке прежние голоса и позиции сохраняются.
Порядок управления определяется порядком вызовов хоста:

1. STOP удаляет **все** текущие голоса этого ID; срабатывает при полной таблице
   и при несовпадении геометрии stop-источника. STOP приоритетнее NO_MULTIPLE.
   Отсутствие совпадающего голоса тоже даёт OK.
2. NO_MULTIPLE при активном совпадающем ID даёт OK без подготовки нового голоса.
   Обычный start разрешает несколько копий одного ресурса.
3. В strict режиме новый голос требует совпадения source rate/channels, иначе UNSUPPORTED.
   [`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L82) применяет диапазон, повторы и огибающую. Каждая
   копия получает независимую позицию чтения и огибающей.
4. Пустой диапазон/явно нулевое число повторов не занимают слот. Иначе заполнение
   всех слотов даёт OUT_OF_MEMORY без кражи или замены прежнего голоса.

Подготовка выделяет память/декодирует и может завершиться ошибкой, в том числе
при обнаружении пустого голоса или полной таблицы. Не делайте её в real-time
callback. Действует существующий **покадровый снимок/один голос** memory limit
исходного runtime, а не общий бюджет микшера. Суммарная декодированная память
может достигать capacity × per-voice limit плюс native служебная память.
Подбирайте оба ограничения. Повторы не дублируют хранимый PCM. Submit не двигает
звуковые часы.

### fvm_swf_sound_mixer_read

`[fvm_swf_sound_mixer_read](../include/flashvm/swf_sound_mixer.h#L72)(mixer, frames, destination, capacity_values,
written_values)` заполняет ровно frames sample frames. Capacity/written считают
**short-значения каналов**, то есть для stereo нужно frames × 2. Written не может
быть null, обнуляется до проверок и не должен перекрываться с destination.
Destination обязателен при frames > 0. Ёмкость проверяется делением до умножения,
включая переполнение size_t. Ошибка аргументов даёт INVALID_ARGUMENT без изменения
голосов/выходного буфера. Нулевое чтение разрешает null destination и не двигает
позиции.

После OK в strict режиме голоса продвинуты не более чем на frames. Для configured
conversion действуют [отдельные часы/lookahead](SWF_SOUND_CONVERSION.ru.md). Короткий/законченный голос
дополняется тишиной; пустой микшер тоже возвращает полный буфер тишины и OK,
а не NOT_FOUND. Закончившиеся snapshots освобождаются, слоты доступны повторно.
Значения с уже применённой огибающей суммируются в signed long ограниченными
порциями; только **вся** сумма насыщается до -32768..32767. Поэтому
30000 + 30000 - 30000 даёт 30000 независимо от порядка голосов. Ограничение 64
голоса удерживает сумму в 32-битном long.

Нет new, доступа к source VM или output callbacks. Однако завершение голосов
освобождает native память; это не обещание жёсткого real-time deadline без
вызовов аллокатора. При таких требованиях используйте producer/очередь устройства.
Время большого запроса растёт пропорционально его длине и числу голосов.

### fvm_swf_sound_mixer_info

`fvm_swf_sound_mixer_info(mixer, out)` копирует [`FVM_SWFSoundMixerInfo`](../include/flashvm/swf_sound_mixer.h#L13):
sample_rate, channels, voice_capacity, active_voices. Сначала установите точный
`struct_size = sizeof(...)`. Null или неверный размер дают INVALID_ARGUMENT,
выход сохраняется. Нет выделений, callbacks и доступа к VM. Active не считает
пустые/законченные выходные голоса; оставшийся converted lookahead ещё считается,
даже если decoder достиг EOF. Active ничего не говорит о PCM, уже ушедшем в устройство.

### fvm_swf_sound_mixer_stop_all

`fvm_swf_sound_mixer_stop_all(mixer)` освобождает все software voices без выделений.
Null даёт INVALID_ARGUMENT; пустой живой микшер — OK. Геометрия/ёмкость сохраняются
для повторного использования. Следующее чтение даёт тишину до нового start.
Функция не останавливает и не сбрасывает внешнюю очередь устройства или FILE.

### fvm_swf_sound_mixer_close

`fvm_swf_sound_mixer_close(mixer)` освобождает голоса и микшер; null допустим.
Возвращает void, после вызова handle недействителен. Живая исходная VM не нужна,
callbacks не вызываются. Устройство хост закрывает отдельно по контракту моста.

## Интеграция с часами и timeline

[Корневой callback](MOVIECLIP_SOUND.ru.md) доставляет реально исполняемые входы;
нельзя подавать все root события на каждом display tick. Хост передаёт их по
порядку и запрашивает PCM по выбранным часам. Повторный start создаёт новый голос,
отсутствие read останавливает software clock. [Configured conversion](SWF_SOUND_CONVERSION.ru.md)
явно обрабатывает разные rate/channels. Sprite sound, stream codecs, band-limited
фильтр и автоматическая device/A/V синхронизация ещё открыты. PCM передаётся в
host bridge без отката уже прочитанных samples при ошибке вывода.

## Проверки и пределы результата

Проверяются mono/stereo, четыре частоты, все перестановки трёх голосов, однократное
насыщение суммы, 64 голоса, ёмкость/повторное использование слотов, stop/no-multiple,
повторы, тишина, ошибочная/переполняющаяся ёмкость и атомарность отказов. ADPCM
2..5 бит проверяется через границу пакета 4096 frames с огибающими/циклами и
порциями 1/7/255/256/257/8192 против существующего полного event rendering.
Инъекции OOM проверяют create/submit и отсутствие новых выделений при
read/info/stop/close; reload/GC/уничтожение VM проверяют native владение.
Отдельный strict C89 клиент вызывает все функции. Настоящий пример проверяется
по точным native PCM байтам, ошибкам CLI и fopen. Это software-проверки, а не
подтверждение слышимого вывода, других ОС или завершения стадий 4/8/9.
## Реальные события root timeline (v0.579)

[MovieClip sound callback](MOVIECLIP_SOUND.ru.md) теперь позволяет подать события
реального входа в кадр без звуков пропущенных кадров при seek reconstruction.
Device clock и автоматическая привязка AVM1 StopSounds всё ещё требуют host моста.

## Необязательное затухание v0.587

[Политика плеера, новые API и ограничения](PLAYER_AUDIO_RELEASE.ru.md). Прежняя немедленная остановка сохраняется по умолчанию.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:62](../include/flashvm/swf_sound.h#L62) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:142](../include/flashvm/swf_sound.h#L142) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:91](../include/flashvm/swf_sound_mixer.h#L91) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:45](../include/flashvm/swf_sound_mixer.h#L45) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:87](../include/flashvm/swf_sound_mixer.h#L87) |
| `FVM_SWF_SOUND_MIXER_MAX_VOICES` | macro / define | [include/flashvm/swf_sound_mixer.h:9](../include/flashvm/swf_sound_mixer.h#L9) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:72](../include/flashvm/swf_sound_mixer.h#L72) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:89](../include/flashvm/swf_sound_mixer.h#L89) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:50](../include/flashvm/swf_sound_mixer.h#L50) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `FVM_SWFSoundMixerInfo` | struct | [include/flashvm/swf_sound_mixer.h:13](../include/flashvm/swf_sound_mixer.h#L13) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
<!-- source-links:end -->

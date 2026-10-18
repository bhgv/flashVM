# Доставка корневых звуковых событий MovieClip

## Поддерживаемый путь

Импортируются корневые DefineSound (14) и StartSound (15). Перед advance звукового
кадра зарегистрируйте [`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21). Callback типа
[`FVM_MovieClipSoundCallback`](../include/flashvm/movieclip.h#L18) получает `(context, clip, event_index, userdata)`
при реальном входе в кадр. Нулевой индекс подходит для [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L58)
и [`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L48) того же загруженного SWF. Хост выбирает микшер,
геометрию PCM и очередь устройства. Скрытых устройств/потоков нет.
[Микшер](SWF_SOUND_MIXER.ru.md), [исполняемый C89 пример](../examples/multimedia/swf_event_mixer/README.md).

## Загрузка и проверка источника

Для SWF с этими тегами сначала вызовите [`fvm_load_memory`](../include/flashvm/flashvm.h#L72), затем
[`fvm_movieclip_load_swf`](../include/flashvm/movieclip.h#L43) с тем же файлом. Эквивалентные FWS/CWS сравниваются как
canonical FWS. Отсутствующий/другой source даёт INVALID_ARGUMENT и null clip.
Проверяются определения, разрешение ресурсов и SOUNDINFO, но импорт не декодирует
и не играет PCM. Кодек может быть принят как метаданные, а затем отвергнут
микшером при исполнении. Sound ID нельзя одновременно использовать как другой
символ или помещать в display list. Звук внутри sprite и SoundStreamHead/Block
по-прежнему UNSUPPORTED. Для файлов без звука порядок старого импорта не меняется.

[GC:] Граф кадров хранит порядковые индексы и полную копию canonical source в ASCII
hex. Native callback/mixer указателей в нём нет. Перед каждой группой событий
звукового кадра байты сравниваются с context source целиком — время линейно от
длины файла. Не используется hash/адрес/поколение, которое могло бы ошибочно
пережить перенос графа. Hex занимает удвоенную длину source плюс managed/контейнерные
расходы; при импорте дополнительно есть временные decoded/string буферы. Старые
decode/graph limits действуют, нового общего memory budget этот API не обещает.
Дальнейший prepared index может уменьшить эту стоимость с сохранением проверки.

После reload другого SWF старый визуальный граф остаётся, но его звук даёт
INVALID_ARGUMENT. Загрузка исходных canonical bytes позволяет повторный явный
вход в кадр. Reload во время callback запрещён. Хост должен root/pin клип.

## Контракт регистрации и callback

`fvm_movieclip_set_sound_callback(context, callback, userdata)` заимствует оба
указателя до замены/уничтожения context; не выделяет память и не освобождает
userdata. Null callback отменяет регистрацию, userdata игнорируется. Для живого
context на его владеющем потоке возвращает OK. Null/терминальная фаза GC/destroy
или регистрация внутри звукового callback дают INVALID_ARGUMENT, прежняя
регистрация сохраняется. Настройка относится к context, отдельно от графов клипов
и context audio channels.

Callback синхронный на VM-потоке под native-call GC barrier. Разрешены запросы
метаданных и submit в host mixer. Нельзя менять граф кадров/клипа, исполнять script
или уничтожать используемые runtime/host ресурсы. Context destroy уже блокируется
native barrier. Рекурсивные [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L54), смена callback и
[`fvm_load_memory`](../include/flashvm/flashvm.h#L72) во время доставки возвращают INVALID_ARGUMENT. Userdata должно
жить до возврата. C++ bad_alloc преобразуется в OUT_OF_MEMORY, прочие исключения —
ERROR, возвращённый статус передаётся без изменения. Busy снимается на любом выходе.

## Порядок исполнения и ошибки

При фактическом входе сначала применяется визуальная сцена, вход отмечается
использованным, доставляются StartSound по порядку source, затем выполняются
DoAction этого кадра. Далее сохраняется существующий порядок onEnterFrame/children.
Восстановление визуального состояния при goto/loop не запускает звуки пропущенных
кадров. Уже введённый остановленный кадр не повторяет звук на каждом host tick.
Явный seek в тот же кадр и естественный цикл создают новый вход и новую доставку.
Звуковые флаги исполняет микшер; callback сам не владеет голосами. Интерпретатор
и JIT используют общий путь доставки перед frame actions.

Если callback не установлен, звуковой вход даёт UNSUPPORTED вместо молчаливого
пропуска. Несовпадение source, OOM или ошибка callback прерывают advance до DoAction.
Уже применённая сцена и предыдущие успешные host callbacks не откатываются. Вход
уже использован: автоматический повтор мог бы продублировать голоса/внешний I/O.
Прервите playback либо явно seek/retry с восстановлением host state. Одна новая
регистрация не повторяет ошибочный кадр. Отката hardware queue, frame skipping,
resampling и hard real-time deadline нет. AVM1 StopSounds всё ещё действует на
context audio channels; очистку независимого mixer/device обеспечивает host stop bridge.

## Перенос и проверки

Frame/source данные переносятся как managed edges. Callback/userdata не
сериализуются; получатель загружает тот же SWF и регистрирует собственный callback
с микшером. Исходный context после переноса можно уничтожить. Это проверяется
через serialization, GC, pin и явный retry после отсутствующего callback.
Также проверяются Stop, goto, повторный вход, loop, порядок событий, reload,
исключения/повторный вход, OOM импорта, duplicate ID, неверные флаги, попытка
placement звука, неподдерживаемый sprite sound, FWS/CWS и реальные interpreter/JIT
счётчики. C89 учебник проверяет manual/timeline режимы по точным PCM-байтам.
Это software-проверки, не слышимый вывод или нативная приёмка других ОС.
Sound ExportAssets/linkage is explicitly UNSUPPORTED until Sound.attachSound
support exists; it is never installed as an undefined visual symbol.
Звуковой ExportAssets/linkage пока явно UNSUPPORTED: Sound.attachSound ещё
не реализован этим изменением. Проверяется отдельным malformed/unsupported fixture.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:54](../include/flashvm/movieclip.h#L54) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../include/flashvm/movieclip.h#L43) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `FVM_MovieClipSoundCallback` | callback typedef | [include/flashvm/movieclip.h:18](../include/flashvm/movieclip.h#L18) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:58](../include/flashvm/swf_sound.h#L58) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:48](../include/flashvm/swf_sound_mixer.h#L48) |
<!-- source-links:end -->

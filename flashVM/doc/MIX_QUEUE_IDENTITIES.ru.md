# Очередные ID при смешивании stream/event

Актуальный контракт: один numeric sound-ID namespace root-словаря на tracked-отсчёт.
Разные SWF/дочерние словари автоматически не различаются. Все native-владельцы и
callbacks сериализуются хостом. Микшер/backend/очередь живут до quiescence;
все start/stop проходят через tracked API. Порядок: clock_begin, tracking, admission.
Смена словаря требует согласованных pause/успешной clear/source selection;
автоматический replay событий не выполняется.

Неверные/NULL/busy аргументы дают INVALID_ARGUMENT; выключенный режим — UNSUPPORTED;
byte-budget/выделения — OUT_OF_MEMORY, временной overflow — UNSUPPORTED. Статусы
backend передаются, исключения становятся ERROR. Ошибка сохраняет native-владение/
дедлайны; корректное наблюдение может обновить played. API не исполняют VM, не
вызывают GC/устройство и не реализуют DAC/A-V/дочернее планирование. Успешная opt-in
adapted.clear после acknowledgement останавливает все mixer-голоса и обнуляет
ID/clock/pending; ошибка сохраняет их. Untracked clear не меняется. Таблица
принадлежит очереди и освобождается при её закрытии либо безопасном disable.

| API | Контракт |
| --- | --- |
| [fvm_swf_sound_mix_queue_set_identity_tracking](../include/flashvm/swf_sound_mix_queue.h#L117) | Включить ограниченную таблицу в нулевом shared-отсчёте; отключить после наблюдённого drain без voices/pending/неистёкших регистраций. Повтор — no-op. Нет backend/VM/GC; одно выделение таблицы, ошибки сохраняют режим. |
| [fvm_swf_sound_mix_queue_submit_prepared_bounded](../include/flashvm/swf_sound_mix_queue.h#L131) | Требует owned prepared handle; успех потребляет его, ошибка сохраняет прогресс/владение. Queued NoMultiple потребляет лишний старт даже при нулевом mixer-budget. Обычный старт продлевает max end после accepted+pending. Нет выделений/VM/GC, poll может обновить played. |
| [fvm_swf_sound_mix_queue_identity_info](../include/flashvm/swf_sound_mix_queue.h#L149) | ID 1..65535 и обязательный out точного размера; копирует sound_id/played/end/until_end/active после реального FIFO poll. Ошибки/выключенный режим сохраняют выход; expiry оставляет исторический end. Нет выделений/VM/GC. |
| [fvm_swf_sound_mix_queue_sound_active](../include/flashvm/swf_sound_mix_queue.h#L154) | Обязательный scalar-out, правила identity_info. True только пока зарегистрированное время не удалено из FIFO; это не резервирование и не DAC-слышимость. |
| [fvm_swf_sound_mix_queue_stop_id](../include/flashvm/swf_sound_mix_queue.h#L162) | Требует допустимый ID и tracking; вызывает immediate/DECLICK stop микшера, отменяет deadline, сохраняет accepted/pending. Нет backend callback, выделений, VM/GC. |
| [fvm_swf_sound_mix_queue_storage_info](../include/flashvm/swf_sound_mix_queue.h#L65) | Точные fixed+buffer=retained запросы; buffer включает обе S16-таблицы и optional deadlines. Allocator/микшер/голоса/backend/VM учитываются отдельно. |
| [fvm_swf_sound_mix_queue_storage_required](../include/flashvm/swf_sound_mix_queue.h#L44) | Предсказывает только исходный untracked adapter; optional таблица добавляет ID_SLOTS*sizeof(double). Ошибка сохраняет scalar-out; нет выделений/callback/VM. |


v0.951 подготавливает защищённое наблюдение очереди для tracked admission: внутренний observe_locked работает только под публичным reentry guard. Обычная adapted.queued сохраняет проверки, сохранность выхода и обработку исключений. Повторно проверяются clock/tail/callback-control. Сам рефакторинг ещё не включает API queued-ID. Native-владельцы сериализуются хостом и не хранят managed/GC-ссылок. Полная desktop-интеграция, устройства/платформы и все стадии открыты.

v0.952: set_identity_tracking добавляет фиксированную таблицу ID_SLOTS. Включение требует активный нулевой shared clock, отсутствие pending PCM и голосов. Точный adapter-budget включает owner, обе S16-таблицы и doubles дедлайнов; нехватка отказывает до выделений. storage_info.buffer_bytes включает optional deadlines, сохраняя fixed+buffer=retained; storage_required предсказывает исходное untracked-создание. Повтор выбранного режима — no-op. Отключение требует наблюдённый drain, отсутствие voices/pending и истёкшие регистрации. Begin/end при включённом режиме отклоняются. Opt-in успешная adapted.clear останавливает tracked-голоса и сбрасывает дедлайны/clock/pending; ошибка ничего не меняет. Untracked-поведение и прежние публичные структуры сохраняются. Конфигурация не вызывает backend/VM/GC.

v0.953: submit_prepared_bounded наблюдает shared FIFO под общим reentry guard, размещает новый phase-zero голос после accepted плюс pending и регистрирует наибольший exclusive ID end. NoMultiple потребляет неиспользованный голос, пока зарегистрированный интервал не удалён из FIFO, включая pending и уже завершённый mixer-голос. После drain разрешён новый старт. Успех потребляет handle; ошибка сохраняет владение/прогресс/старые voices/deadlines, корректный poll может обновить played. Переполнение длительности — UNSUPPORTED; mixer_budget ограничивает только микшер/голоса. Выключенный tracking — UNSUPPORTED. Тесты сохраняют шаблоны после source VM teardown и сравнивают точный PCM до/после drain.

v0.954: identity_info возвращает копию ID/dequeue/end/until-end/active точного размера; sound_active использует тот же путь. Нужны ID 1..65535, живой сериализованный owner и точный out. Выключенный режим — UNSUPPORTED; ошибки аргументов/reentry/backend сохраняют выходы; нет выделений/VM/GC. Истёкшие дедлайны видны исторически. Тест overlap доказывает старт после pending, продление end с 5 до 7 без повторного смешивания; нулевой mixer-budget сохраняет handle/прогресс/deadline. Проверены независимый PCM-oracle на семь samples, частичный/full drain и неверные/незарегистрированные ID.

v0.955: stop_id отменяет будущее same-ID смешивание и дедлайн через immediate/DECLICK микшера, не меняя accepted/pending PCM. Старый pending-блок остаётся точным; новый NoMultiple начинается после него. Ошибка opt-in clear сохраняет pending/голоса/deadline; успешная clear останавливает tracked-голоса и сбрасывает deadlines/clock/pending. Затем проходят безопасное отключение и clock_end. Stop ID не отзывает уже поставленный audible PCM; полная отмена очереди явная.

v0.956: callbacks пробуют рекурсивные admission/query/stop/mode с настоящим prepared probe; все сохраняют его handle и прогресс. Ошибка наблюдения, исключения write/queued/clear и нулевой mixer-budget сохраняют регистрации/владение. Исключения проверяются обычным тестом, --no-throw доступен санитайзерам. Счётчик выделений доказывает: недостаточный adapter-budget не делает new, отказ таблицы сохраняет режим, одно выделение включает tracking. При запрете выделений adoption/NoMultiple/query/tail retry/drain/stop/clear/disable/clock_end не делают new.

v0.957: настоящий встроенный SWF даёт преобразуемый root stream и два независимо prepared ID после source VM teardown. Tracked admission регистрирует end на output-кадрах 9 и 14; ID8 активен после stream EOF и отказа tail. Точный stereo PCM не меняется; общий выход заканчивается на 15 с padding, истёкшие регистрации сохраняют дедлайны, drained disable/end проходит. Отдельно сохраняются gate/pending/exception проверки. Это native composition-приёмка, не production desktop и не устройства.

v0.958: четыре настоящих OS-потока выполняют по 64 независимых tracked-отсчёта с разным PCM и native-шаблонами после source VM teardown. Каждый отклоняет/повторяет выход, подавляет NoMultiple, сохраняет ID-active после mixer retirement, проверяет PCM, подтверждает drain и clear-сброс ID/clock. Таблицы локальны и переиспользуются; handles VM/микшера/очереди/голоса/шаблона не разделяются. Безопасность общих изменяемых handles не заявляется.



v0.960: настоящие root MovieClip callbacks принимают ordinary/NoMultiple и маршрутизируют stop-ID через tracked API. Интерпретатор и native JIT выполняют frame-actions с entry counters; stopped ticks и GC safe points сохраняют доставку. Завершённый queued-ID активен, stop отменяет lease без удаления FIFO PCM, новый root-start следует за accepted. Точный stereo PCM переживает VM teardown. Перед моделью drain выполняется явный resume, затем disable/end. В JIT-профиле прошли оба режима. Это управляемая root callback-приёмка, не полный desktop/seek/дочерние клипы.

v0.961: настоящий C89-тест линкует все новые API и принимает/наблюдает/останавливает/очищает native-голоса после VM teardown, проверяя сохранность error-out. White-box seed проверяет MAX_CLOCK и переполнение старта после pending без заявления воспроизведения такой длительности; отказ сохраняет входной прогресс, NoMultiple потребляет лишний старт, clear разрешает новый отсчёт. Finished-голос не регистрирует интервал; частично прочитанный сохраняет курсор. DECLICK cancellation разрешает NoMultiple при ещё живом release, проверяется точный первый overlap PCM. Политики отдельно описаны в тестах и API.


## v0.962 validation

Идентификаторы смешанной очереди v0.951–v0.962: optional-таблица с точным бюджетом сохраняет SyncNoMultiple для pending/FIFO-звука после удаления голоса из микшера. Проверены prepared admission, наблюдение, stop, подтверждённый clear и владение retry. Область — один root-словарь; дочерние словари не перенумеровываются. Прошли 125 native проверок с примером, 34 выбранных ASan/UBSan и семь --no-throw запусков; root-callback тест выполнен интерпретатором и native JIT с GC и проверкой native entries. Исключения и сбои выделений проверены обычной сборкой; LeakSanitizer отключён. Переименованный отделённый компонент сконфигурирован, его скопированный PCM-checker прошёл на native-бинарнике; отдельной сборки не было. Desktop-интеграция stream/event, дочерние таймлайны, автоматические A/V/seek, устройства/платформы и все девять стадий остаются открытыми. [Подробнее](MIX_QUEUE_IDENTITIES.ru.md).


v0.963: Добавлен немедленный stop_all событий: pending/FIFO PCM и общий счётчик сохраняются.


v0.964: Добавлен точный storage_required_for_tracking без создания владельца и callbacks.


v0.965: Проверены точный base/tail retry и новый NoMultiple после сохранённого pending PCM.


v0.966: stop_all отвергает callback-reentry; независимый расчёт памяти разрешён внутри callback.


v0.967: Проверены удаление DECLICK-хвостов и новый NoMultiple после сохранённого retry.


v0.968: Проверены отмена двух root-ID и независимые сроки новых голосов после принятого PCM.


v0.969: При нулевом allowance проверены new/new[] для расчёта памяти, нового голоса и stop_all.


v0.970: Выполнены 256 stop/retry/restart циклов на четырёх OS-потоках с независимыми PCM и ID.


v0.971: Исполняемый C89-пример использует точный бюджет с ID и общую остановку событий.


v0.972: Проверены root StopSounds, failed clear/retry, GC и interpreter/JIT; event stop_all сохраняет FIFO.


v0.973: Проверено продолжение настоящего stream после stop_all событий и точный retry готового микса.

## Общая остановка событий и расчёт памяти адаптера

[`fvm_swf_sound_mix_queue_stop_all`](../include/flashvm/swf_sound_mix_queue.h#L170) немедленно удаляет все голоса микшера, включая
DECLICK-хвосты, и сбрасывает сроки всех ID. В отличие от
[`fvm_swf_sound_mix_queue_stop_id`](../include/flashvm/swf_sound_mix_queue.h#L162), общая остановка не начинает release-ramp.
Принятый и pending PCM сохраняется побайтно, как и курсор base-producer,
состояние паузы и общий отсчёт. Новый NoMultiple разрешён сразу, но начинает
смешиваться после уже подготовленных pending-кадров. Повтор безопасен.
Это управление будущими событиями, а не очистка устройства или полный StopSounds.

Для context StopSounds хост регистрирует callback с подтверждённым `adapted.clear`
и согласует его с base-producer. Только успешный clear удаляет pending/FIFO,
голоса, ID и общий отсчёт. Ошибка clear сохраняет всё это состояние.
После остановки только событий нельзя сообщать об отмене всего вывода.
Root-тест использует настоящий AVM1 StopSounds, ошибку clear, повтор, закреплённый
MovieClip, GC safe points и исполнение interpreter/native JIT.

[`fvm_swf_sound_mix_queue_storage_required_for_tracking`](../include/flashvm/swf_sound_mix_queue.h#L52) заранее вычисляет точный
объём запросов адаптера для обоих режимов. Channels — 1 или 2, block — в существующем
пределе вывода, tracking — строго 0/1. Обязательный выходной scalar сохраняется
при неверных аргументах и переполнении. Нет callbacks, выделений, VM или GC;
вызывать расчёт можно и из backend callback. Включены владелец, оба PCM-массива
и optional ID-таблица; память allocator/device/mixer/voices/VM считается отдельно.
Создание по-прежнему начинается без tracking; таблица включается отдельно в
нулевой эпохе без голосов. Расчёт не резервирует память и не гарантирует её выделение.

| API | Ошибки, владение и сериализация |
| --- | --- |
| [`fvm_swf_sound_mix_queue_stop_all`](../include/flashvm/swf_sound_mix_queue.h#L170) | Нужны живой tracked queue и заимствованные владельцы, внешняя сериализация всех операций микшера/очереди. Null/reentry: INVALID_ARGUMENT; tracking отключён: UNSUPPORTED. Ошибка микшера предшествует изменению ID. Без callbacks/выделений/VM/GC, передачи владения и неявных pause/clear. |
| [`fvm_swf_sound_mix_queue_storage_required_for_tracking`](../include/flashvm/swf_sound_mix_queue.h#L52) | Обязательный scalar out, допустимые геометрия и режим. Неверные аргументы: INVALID_ARGUMENT; переполнение размера: OUT_OF_MEMORY; выход сохраняется. Владелец не нужен; параллельным вызовам нужны независимые выходные ячейки. |

Новые native-тесты проверяют принятый PCM, pending BASE/TAIL retry, разные
конвертированные ID, удаление release-хвостов, немедленный повтор NoMultiple,
отсутствие выделений и четыре OS-потока по 64 цикла. Потоки владеют отдельными
адаптерами; несинхронизированный доступ к общему микшеру не заявлен.


## v0.974 regression evidence

Общее управление событиями v0.963–v0.974: tracked stop_all немедленно удаляет все event/release-голоса и ID-сроки, сохраняя pending/FIFO PCM, base-курсор и общий счётчик. Добавлен точный расчёт памяти для выбранного режима до создания адаптера. Проверены BASE/TAIL retry, новый NoMultiple, разные конвертированные ID, release, отсутствие выделений, четыре независимых OS-потока и настоящий root StopSounds/GC/JIT binding. Прошли 134 native проверки с примером, 43 выбранных ASan/UBSan и семь --no-throw запусков. Отдельный native JIT-профиль проверяет interpreter/native entries root StopSounds. Исключения и allocation-failures проверены обычной сборкой; LeakSanitizer отключён. Отделённый переименованный компонент сконфигурирован, скопированный PCM-checker прошёл на native-бинарнике; отдельной сборки не было. Desktop mixing, дочерние словари/таймлайны, автоматические A/V/seek, реальные устройства/платформы и все девять стадий остаются открытыми. [Контракт](MIX_QUEUE_IDENTITIES.ru.md).

The converted-stream cancellation oracle uses a single positive event so its retained first block clips; later samples are checked against independent base-only rational interpolation. / Оракул отмены stream использует один положительный event: первый сохранённый блок насыщается, последующие отсчёты сверяются с независимой рациональной интерполяцией только base.


v0.974: Описаны tracked stop_all, расчёт памяти, подтверждённый StopSounds и границы проверок.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_identity_info` | function | [include/flashvm/swf_sound_mix_queue.h:149](../include/flashvm/swf_sound_mix_queue.h#L149) |
| `fvm_swf_sound_mix_queue_set_identity_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:117](../include/flashvm/swf_sound_mix_queue.h#L117) |
| `fvm_swf_sound_mix_queue_sound_active` | function | [include/flashvm/swf_sound_mix_queue.h:154](../include/flashvm/swf_sound_mix_queue.h#L154) |
| `fvm_swf_sound_mix_queue_stop_all` | function | [include/flashvm/swf_sound_mix_queue.h:170](../include/flashvm/swf_sound_mix_queue.h#L170) |
| `fvm_swf_sound_mix_queue_stop_id` | function | [include/flashvm/swf_sound_mix_queue.h:162](../include/flashvm/swf_sound_mix_queue.h#L162) |
| `fvm_swf_sound_mix_queue_storage_info` | function | [include/flashvm/swf_sound_mix_queue.h:65](../include/flashvm/swf_sound_mix_queue.h#L65) |
| `fvm_swf_sound_mix_queue_storage_required` | function | [include/flashvm/swf_sound_mix_queue.h:44](../include/flashvm/swf_sound_mix_queue.h#L44) |
| `fvm_swf_sound_mix_queue_storage_required_for_tracking` | function | [include/flashvm/swf_sound_mix_queue.h:52](../include/flashvm/swf_sound_mix_queue.h#L52) |
| `fvm_swf_sound_mix_queue_submit_prepared_bounded` | function | [include/flashvm/swf_sound_mix_queue.h:131](../include/flashvm/swf_sound_mix_queue.h#L131) |
<!-- source-links:end -->

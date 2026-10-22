# Общий счётчик FIFO смешивания

v0.939 добавляет явно включаемый счётчик в native-адаптер stream/event. [fvm_swf_sound_mix_queue_clock_begin](../include/flashvm/swf_sound_mix_queue.h#L69) требует живой сериализованный адаптер, отсутствие pending retry и пустую настоящую FIFO. После успешного наблюдения backend начинается новый нулевой отсчёт. Создание остаётся без callbacks, учёт выключен до begin. Пустой дренированный отсчёт можно начать заново. Pause, clear, seek конвертера и сброс голосов автоматически не выполняются.

[fvm_swf_sound_mix_queue_clock_info](../include/flashvm/swf_sound_mix_queue.h#L86) заполняет структуру точного размера [FVM_SWFSoundMixQueueClock](../include/flashvm/swf_sound_mix_queue.h#L52): submitted_frames учитывает каждый реально принятый блок adapted.write и pump_tail; played_frames — submitted минус свежий проверенный queued_frames; seconds делит удалённые из FIFO кадры на частоту выхода. Отказы записи не учитываются до успешного точного retry. Максимум точного счётчика [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14); переполнение даёт UNSUPPORTED до render/write. Это не DAC timestamp, EOF источника, разрешение кадра или полная A/V-синхронизация.

Невозможный рост FIFO сверх submitted/предела backlog и регресс played дают ERROR без публикации выхода и изменения последней корректной позиции. До begin info даёт NOT_FOUND без callbacks. Неверные размер/NULL/busy дают INVALID_ARGUMENT с сохранением выхода. Статусы backend передаются, исключения становятся ERROR. Успешная adapted.clear обнуляет активный отсчёт; ошибка clear сохраняет счётчики/pending, pause их не меняет. Владелец сериализует вызовы и сохраняет микшер/backend живыми. На hot path нет выделений и VM/GC-ссылок. Прежние публичные структуры не меняются; storage-query включает новые private-поля владельца.

Базовый тест проверяет явное включение, точный учёт отказа/retry, удаление из FIFO и новый отсчёт. Production desktop-интеграция, устройства/платформы и все девять стадий остаются открытыми.

v0.940: clock_end явно выключает учёт после остановки производителей, разрешения pending и проверенного опустошения FIFO. Неактивный/непустой счётчик даёт NOT_FOUND, pending/NULL/busy — INVALID_ARGUMENT; ошибки backend передаются. Корректный непустой poll может обновить played, оставляя учёт активным. Нет автоматических pause/clear, сброса голосов/источника и передачи владения. Тесты проверяют pending/end, непрослушанный/end, drained/end, прежнее проксирование очереди после end и новый begin.

v0.941: callbacks пробуют рекурсивные begin/end/info при queued/write/pause/clear; все отклоняются до публикации. Ошибка begin оставляет счётчик неактивным; ошибки write/clear/pause и исключения write/queued сохраняют retry/учёт. Невозможный рост и регресс played сохраняют снимок вызывающего и последнюю корректную позицию. Успешная clear сбрасывает активный отсчёт. Обычный тест покрывает исключения; --no-throw доступен для отдельного запуска санитайзеров.

v0.942: явный white-box seed проверяет MAX_CLOCK-2, отказ двухкадрового блока, точный retry на MAX_CLOCK и отказ переполнения base/tail без движения голоса и backend write. После успешной clear реальный tail начинается со следующего source sample, счётчик — с нуля. Тест компилирует настоящую реализацию и задаёт только счётчики; воспроизведение длительности 2^53 кадров не заявляется.

v0.943: настоящий преобразуемый SWF stream-producer даёт девять принятых кадров; отказ tail не меняет сумму, точные повторы доводят общий приём/удаление до 15 кадров. Stream-only producer по-прежнему сообщает свои девять. Frame-gate, EOF и опустошение очереди различаются. Сохранены полный stereo PCM-oracle и удаление исходной VM; drained clock_end проходит. Это native-учёт перехода, не frontend A/V-синхронизация.

v0.944: настоящие MovieClip frame-actions выполняются интерпретатором и доступным native JIT с проверкой native-entry counters. Общий учёт следует разрешённым stream-кадрам, остановке/gap и GC safe points. Явные goto/stream seek вызывают clear адаптера и начинают согласованный нулевой отсчёт; проверяются повторные source bytes и итоговые метрики. В native JIT-профиле выполнены оба режима. Это ограниченная root timeline/seek-проверка, не полная Flash seek-совместимость и не дочерние клипы.

v0.945: инструментированный обычный тест по-прежнему отклоняет недостаточный byte-budget до new/new[] и проверяет три границы создания. При запрете выделений clock begin/info/end/restart, tail отказ/retry, приём base, dequeue, pause и clear не делают попыток. Новые private-счётчики включены в required/observed fixed storage без роста hot-path буферов.

v0.946: четыре настоящих OS-потока независимо выполняют по 64 общих отсчёта после удаления source VM. Каждый блок отклоняется/повторяется, PCM остаётся независимым; submitted=8 и queued=2 дают played=6, модель дренирования — played=8 и успешный clock_end. Владельцы/шаблоны/микшеры/backend не разделяются; безопасность общих изменяемых handles не заявляется.

v0.948: три настоящих native stream-сегмента сохраняют общий accepted при непрерывном binding без очистки FIFO. Явный discontinuity/rebind сбрасывает отсчёт только после успешной clear; ошибка сохраняет прежний converter, pending PCM, BASE-владение и счётчики. Последующий успешный binding принимает в новый отсчёт, EOF-binding снова обнуляет его. Обычный тест запрещает выделения при переходах; автоматический seek/replay событийных голосов не выдумывается.

v0.949: настоящий C89-тест создаёт bounded mono-адаптер, начинает общий учёт, принимает PCM, наблюдает dequeue/seconds и завершает только после drain. Полные снимки сравниваются побайтно после ошибок NULL/неактивности/размера. Три новые функции линкуются в minimal NO_MEDIA/NO_FONTS; прежние публичные структуры и ABI backend не меняются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_clock_begin` | function | [include/flashvm/swf_sound_mix_queue.h:69](../include/flashvm/swf_sound_mix_queue.h#L69) |
| `fvm_swf_sound_mix_queue_clock_info` | function | [include/flashvm/swf_sound_mix_queue.h:86](../include/flashvm/swf_sound_mix_queue.h#L86) |
| `FVM_SWF_SOUND_OUTPUT_MAX_CLOCK` | macro / define | [include/flashvm/swf_sound_output.h:14](../include/flashvm/swf_sound_output.h#L14) |
| `FVM_SWFSoundMixQueueClock` | struct | [include/flashvm/swf_sound_mix_queue.h:52](../include/flashvm/swf_sound_mix_queue.h#L52) |
<!-- source-links:end -->

# Общий счётчик FIFO смешивания

v0.939 добавляет явно включаемый счётчик в native-адаптер stream/event. [fvm_swf_sound_mix_queue_clock_begin](../include/flashvm/swf_sound_mix_queue.h#L69) требует живой сериализованный адаптер, отсутствие pending retry и пустую настоящую FIFO. После успешного наблюдения backend начинается новый нулевой отсчёт. Создание остаётся без callbacks, учёт выключен до begin. Пустой дренированный отсчёт можно начать заново. Pause, clear, seek конвертера и сброс голосов автоматически не выполняются.

[fvm_swf_sound_mix_queue_clock_info](../include/flashvm/swf_sound_mix_queue.h#L86) заполняет структуру точного размера [FVM_SWFSoundMixQueueClock](../include/flashvm/swf_sound_mix_queue.h#L52): submitted_frames учитывает каждый реально принятый блок adapted.write и pump_tail; played_frames — submitted минус свежий проверенный queued_frames; seconds делит удалённые из FIFO кадры на частоту выхода. Отказы записи не учитываются до успешного точного retry. Максимум точного счётчика [FVM_SWF_SOUND_OUTPUT_MAX_CLOCK](../include/flashvm/swf_sound_output.h#L14); переполнение даёт UNSUPPORTED до render/write. Это не DAC timestamp, EOF источника, разрешение кадра или полная A/V-синхронизация.

Невозможный рост FIFO сверх submitted/предела backlog и регресс played дают ERROR без публикации выхода и изменения последней корректной позиции. До begin info даёт NOT_FOUND без callbacks. Неверные размер/NULL/busy дают INVALID_ARGUMENT с сохранением выхода. Статусы backend передаются, исключения становятся ERROR. Успешная adapted.clear обнуляет активный отсчёт; ошибка clear сохраняет счётчики/pending, pause их не меняет. Владелец сериализует вызовы и сохраняет микшер/backend живыми. На hot path нет выделений и VM/GC-ссылок. Прежние публичные структуры не меняются; storage-query включает новые private-поля владельца.

Базовый тест проверяет явное включение, точный учёт отказа/retry, удаление из FIFO и новый отсчёт. Production desktop-интеграция, устройства/платформы и все девять стадий остаются открытыми.

v0.940: clock_end явно выключает учёт после остановки производителей, разрешения pending и проверенного опустошения FIFO. Неактивный/непустой счётчик даёт NOT_FOUND, pending/NULL/busy — INVALID_ARGUMENT; ошибки backend передаются. Корректный непустой poll может обновить played, оставляя учёт активным. Нет автоматических pause/clear, сброса голосов/источника и передачи владения. Тесты проверяют pending/end, непрослушанный/end, drained/end, прежнее проксирование очереди после end и новый begin.

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

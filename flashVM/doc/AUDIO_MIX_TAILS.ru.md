# Доигрывание событий через очередь смешивания

v0.927 добавляет [fvm_swf_sound_mix_queue_pump_tail](../include/flashvm/swf_sound_mix_queue.h#L63)(queue, frames, target, accepted). После остановки производителя stream/base владелец явно разрешает один событийный блок с нулевой основой. frames положителен и не больше ёмкости адаптера; target от frames до [FVM_SWF_SOUND_OUTPUT_MAX_QUEUE](../include/flashvm/swf_sound_output.h#L12). Часы, дедлайны, пауза, seek и переход EOF не угадываются.

Перед смешиванием читается настоящая длина очереди. Недостаток места либо отсутствие активных голосов/pending-tail дают NOT_FOUND. Ошибка наблюдения и backpressure не двигают голоса. Последний блок может дополниться тишиной; исчерпание голосов не означает опустошение очереди. Необязательный accepted сначала обнуляется и получает frames только после реального приёма.

Отказ записи сохраняет точные смешанные байты и уже потреблённое время голосов. Повторять нужно тот же размер. Pending base PCM либо другой размер tail дают INVALID_ARGUMENT до callbacks. Обычная adapted.write не может принять tail-retry даже при нулевых входных байтах. Успешная clear отменяет pending без перемотки голосов; pause сохраняет его. Ошибки backend передаются, исключения становятся ERROR, рекурсивные callbacks дают INVALID_ARGUMENT.

На пути pump нет выделений, VM и корней GC. Адаптер, микшер и backend userdata живут до завершения всех вызовов; владелец сериализует доступ и останавливает производителей перед close. Прежние публичные структуры и stream-контракты не меняются. Детерминированный тест переживает исходную VM и проверяет точный PCM при отказе/повторе, backpressure и последний дополненный блок. Desktop-интеграция, устройства/платформы и полная приёмка SWF остаются открытыми.

v0.928: [fvm_swf_sound_mix_queue_pending](../include/flashvm/swf_sound_mix_queue.h#L31) копирует новую структуру Pending точного размера с frames и источником NONE/BASE/TAIL. Нет callbacks/выделений/VM/GC и изменения времени; наблюдение не резервирует очередь. Ошибки аргументов/рекурсия сохраняют выход. Тесты доказывают оба направления разделения retry даже для одинаковых нулевых байтов и исчерпанного микшера; другой размер tail не перехватывает pending.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pending` | function | [include/flashvm/swf_sound_mix_queue.h:31](../include/flashvm/swf_sound_mix_queue.h#L31) |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:63](../include/flashvm/swf_sound_mix_queue.h#L63) |
| `FVM_SWF_SOUND_OUTPUT_MAX_QUEUE` | macro / define | [include/flashvm/swf_sound_output.h:12](../include/flashvm/swf_sound_output.h#L12) |
<!-- source-links:end -->

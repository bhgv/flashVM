# Заполнение неполного остатка звуковой очереди

[`fvm_swf_sound_mix_queue_pump_tail_up_to`](../include/flashvm/swf_sound_mix_queue.h#L222) принимает живую нативную очередь,
максимальное число кадров блока, положительную целевую глубину FIFO и необязательный
счётчик принятых кадров. Максимум ограничен ёмкостью адаптера; целевая глубина может
быть меньше максимума. При активных голосах создаётся min(максимум, свободный остаток)
кадров. Полная очередь или отсутствие голосов дают NOT_FOUND без продвижения голосов.

Отклонённый TAIL неделим: меньший максимум или недостаточный остаток дают NOT_FOUND,
сохраняя размер и байты retry. При достаточном месте повторяется исходный блок,
даже если максимум изменился. Удерживаемый BASE даёт INVALID_ARGUMENT. Старый
[`fvm_swf_sound_mix_queue_pump_tail`](../include/flashvm/swf_sound_mix_queue.h#L206) сохраняет точный размер блока и прежний контракт.
accepted сначала обнуляется, затем получает фактически принятые кадры только при OK.
Неверные параметры/рекурсия дают INVALID_ARGUMENT; статусы backend передаются,
исключения дают ERROR, переполнение точных общих часов — UNSUPPORTED до смешивания.
Успешное наблюдение FIFO может обновить played даже при неуспешном результате.

Хост останавливает базовый producer и сериализует очередь, микшер и эксклюзивный
backend; все заимствованные владельцы живут до окончания callbacks. Аллокаций,
VM, GC, неявных pause/clear/seek и передачи владения нет. Последний блок может
содержать padding. Это не определение EOF или физической слышимости. Для общих
со stream часов используется парный планировщик. Дочерние таймлайны/словари,
эталонная A/V/codec/seek-приёмка, устройства и все девять этапов остаются открытыми.

v0.999: Добавлено заполнение остатка FIFO с неделимым retry и сохранением старого API точного размера.


v0.1000: Парный планировщик EOF/gates заполняет остаток FIFO; короткий TAIL сохраняет приоритет перед новым BASE.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:206](../include/flashvm/swf_sound_mix_queue.h#L206) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:222](../include/flashvm/swf_sound_mix_queue.h#L222) |
<!-- source-links:end -->

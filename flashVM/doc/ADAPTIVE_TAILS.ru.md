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


v0.1001: Парные часы получают наблюдение FIFO непосредственно перед write без ошибочного опроса после принятия.


v0.1002: Проверены ошибки backend, рекурсия, исключения, запрет BASE и подтверждённая отмена коротких retry.


v0.1003: Проверены последний точный кадр, отказ при переполнении, неверный FIFO и подтверждённый сброс эпохи.


v0.1004: После подготовки запрещены все аллокации при коротких блоках, retry, смене gate, остановке событий и seek/clear.


v0.1005: Проверены 256 эпох после удаления VM: FIFO по одному кадру, короткие retry, открытие gate, точный PCM и истечение ID.


v0.1006: Production C89 worker проверен в interpreter/native JIT с GC, коротким retry, точным PCM и возобновлением BASE.


v0.1007: Добавлен C89 headroom-режим: 14 коротких retry, неизменный source cursor и общий drain часов и ID.


v0.1008: Независимый PCM-оракул проверяет восемь режимов, 14 кадров headroom, retry и queued/drained lifetime ID.


## Парный планировщик и desktop

[`fvm_swf_stream_output_pump_mix_until`](../include/flashvm/swf_stream_output.h#L104) и [`fvm_swf_stream_output_pump_mix_tail`](../include/flashvm/swf_stream_output.h#L85)
используют адаптивный TAIL с прежними условиями gate/наблюдаемого EOF. Короткий
retry предшествует новому BASE. Stream pending остаётся только базовым; event retry
наблюдается через mix pending. Cursor, phase, chunk и EOF источника не меняются.
Обе копии часов получают наблюдение FIFO непосредственно перед успешным write;
после принятия ошибочного callback нет. Это согласованное наблюдение допуска
блока, не физический DAC timestamp: backend может продолжать потребление.

Desktop root worker наследует этот путь при future gate, authored Stop и EOF.
Host pause останавливает весь вывод. BASE retry не сокращается и не заменяется;
принятый PCM нельзя выборочно отозвать. Последний padding зависит от геометрии
block/остатка; однокадровый вывод может закончиться точно на длительности событий.
Автокоррекция A/V, дочерние таймлайны и эталонный seek этим не реализованы.


v0.1009: Уточнены размеры по остатку FIFO, неделимый retry, admission-clock, desktop pause и ограничения восьми режимов.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mix_queue_pump_tail` | function | [include/flashvm/swf_sound_mix_queue.h:206](../include/flashvm/swf_sound_mix_queue.h#L206) |
| `fvm_swf_sound_mix_queue_pump_tail_up_to` | function | [include/flashvm/swf_sound_mix_queue.h:222](../include/flashvm/swf_sound_mix_queue.h#L222) |
| `fvm_swf_stream_output_pump_mix_tail` | function | [include/flashvm/swf_stream_output.h:85](../include/flashvm/swf_stream_output.h#L85) |
| `fvm_swf_stream_output_pump_mix_until` | function | [include/flashvm/swf_stream_output.h:104](../include/flashvm/swf_stream_output.h#L104) |
<!-- source-links:end -->

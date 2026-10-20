# Потокобезопасный ящик команд плеера

[English](PLAYBACK_MAILBOX.md) · [Команды владельца](PLAYBACK_CONTROL.ru.md) ·
[Владение SDK](PLAYER_HOST_OWNERSHIP.ru.md) · [План](ROADMAP_STATUS.md).

## Нативная основа с v0.713

[FVM_DesktopPlaybackMailbox](../frontend/common/playback_mailbox.h#L12) хранит один mutex, последний запрос и копию скаляров/
текста. Он не хранит context/root, управляемые значения, SDL-окно или callback
вызывающего. Это интерфейс для контролирующего потока IDE: только владелец
плеера входит в VM. Основа и её native-тесты добавлены в v0.713; адаптеры запуска
описаны отдельно по мере их подключения.

Ящик передаёт последнее намерение, а не очередь инструкций. PAUSE, RESUME и STEP
заменяют предыдущую ожидающую неокончательную команду. KEEP ничего не заменяет.
Несколько STEP до приёма объединяются; один принятый STEP разрешает один будущий
такт по общему контракту управления. STOP заменяет всё и остаётся окончательным.
Для N гарантированных шагов сначала дождитесь снимка завершённого шага, затем
отправляйте следующий; серия быстрых post(STEP) не означает N тактов.

## API и ошибки

[fvm_desktop_playback_snapshot_init](../frontend/common/playback_mailbox.h#L32)(out)` заполняет точный struct_size и нули;
возвращает 1 для ненулевого out, 0 для null. Не выделяет память и не запускает SDK.
[FVM_DesktopPlaybackSnapshot](../frontend/common/playback_mailbox.h#L14) — полностью независимая копия, включая source с NUL.
Её можно хранить после возврата/освобождения запуска и самого ящика. В ней нет
скрытых указателей VM. paused/step_pending описывают последнее наблюдение владельца
до применения возвращённой команды; pending/stop_requested читаются при poll.
Это не мгновенное подтверждение применения post и не заморозка VM.

[fvm_desktop_playback_mailbox_create](../frontend/common/playback_mailbox.h#L38)(out)` требует адрес выходного указателя,
возвращает OK, INVALID_ARGUMENT, OUT_OF_MEMORY или ERROR при отказе native mutex/
квоты. При ошибке *out не меняется. Создаёт только native-память и mutex: без VM,
окна, звукового устройства, сетевого запроса или worker. Одновременно разрешено
не более [FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES](../frontend/common/playback_mailbox.h#L5) объектов. Резерв квоты
откатывается при любом отказе. Один ящик предназначен для одного запуска.

[fvm_desktop_playback_mailbox_post](../frontend/common/playback_mailbox.h#L47)(mailbox, command)` потокобезопасно заменяет
ожидающее намерение. Возвращает OK, INVALID_ARGUMENT для null/неизвестной команды
или ERROR при отказе lock и для любой команды кроме STOP после STOP. Повторный
STOP успешен. Память не выделяется, VM и callbacks не вызываются под mutex.

[fvm_desktop_playback_mailbox_poll](../frontend/common/playback_mailbox.h#L52)(mailbox, out)` требует точный struct_size.
Возвращает OK с согласованной копией; NOT_FOUND до первого наблюдения владельца;
INVALID_ARGUMENT для null/неверного размера; ERROR при отказе lock. При любой
ошибке весь out, включая padding, остаётся прежним. Блокируется только на коротком
native mutex, не ждёт кадра/сети и не входит в VM. Скалярные счётчики сохраняют
обычные правила текущего корня и всего запуска из PLAYBACK_CONTROL.ru.md.

[fvm_desktop_playback_mailbox_stop_requested](../frontend/common/playback_mailbox.h#L57)(mailbox)` читает окончательную отмену
без потребления команды. Возвращает 1 для STOP, null или ошибки mutex; иначе 0.
Доступен до появления первого снимка, в том числе при начальной загрузке.

[fvm_desktop_playback_mailbox_active](../frontend/common/playback_mailbox.h#L69)()` возвращает число живых native-объектов без
запуска SDK. [fvm_desktop_shutdown](../frontend/common/player.h#L53)()` возвращает ERROR, пока остаётся хотя бы один
ящик, даже без открытого окна/задания; активный SDK при этом не изменяется.

[fvm_desktop_playback_mailbox_release](../frontend/common/playback_mailbox.h#L42)(mailbox)` возвращает OK, null тоже допустим.
До вызова обязаны завершиться runner и все потоки post/poll/прочих обращений.
Функция не дожидается чужих потоков. Одновременное использование/освобождение,
повторное освобождение и гонка create/release с общим shutdown запрещены.
[GC:] Управляемых корней здесь нет; освобождение ящика не продлевает жизнь VM.

## Проверки и границы

`frontend/tests/playback_mailbox.c` проверяет native mutex без SDL subsystem startup,
согласованность копий при четырёх OS-потоках, приоритет STOP, coalescing, длину
источника, неизменность выхода, квоту и защиту shutdown. Отдельный
`playback_mailbox_allocation.c` инжектирует отказы native-памяти/mutex и проверяет
откат квоты. Это bounded-протокол, не длительное stress/реальное GUI-тестирование.
Нативные запуски остальных ОС/устройств и все девять стадий плана остаются открытыми.

## Адаптеры запуска с v0.714

[fvm_desktop_playback_mailbox_notify](../frontend/common/playback_mailbox.h#L62)(state, userdata)` — callback владельца для
control.notify; userdata равен ящику. Копирует наблюдение и возвращает намерение,
не сохраняя и не разыменовывая заимствованные context/root. Неверный снимок/
handle/source приводит к STOP. Без ожидающей команды возвращается KEEP.
Публикация потребляет неокончательную команду; STOP остаётся для следующих вызовов.

[fvm_desktop_playback_mailbox_load_notify](../frontend/common/playback_mailbox.h#L66)(event, userdata)` — callback владельца
для load monitor. Возвращает ту же непотребляемую отмену, включая время до READY
и блокирующее ожидание provider; null event/userdata или ошибка lock останавливают.
PAUSE/RESUME/STEP ждут playback-уведомления и не приостанавливают provider.
Возвращаемое значение на terminal-событиях, как прежде, игнорируется. Чтобы отменять
старт до импорта, задайте явный background provider и оба callback, а не ждите
playback READY до загрузки.

```c
fvm_desktop_playback_control_init(&control);
control.initially_paused = 1;
control.notify = fvm_desktop_playback_mailbox_notify;
control.userdata = mailbox;
fvm_desktop_load_monitor_init(&monitor);
monitor.notify = fvm_desktop_playback_mailbox_load_notify;
monitor.userdata = mailbox;
/* Передайте control/monitor и явный provider управляемому фоновому запуску. */
```

Это эскиз привязки; исполняемый пример описывает полную очистку. OK от post означает
передачу native-намерения, не завершение такта VM. Снимок содержит состояние до
применения возвращённой команды; подтверждение новой паузы/такта появляется позже.
STOP кооперативен: не прерывает выполняющуюся инструкцию/native callback, DNS или
некооперативный provider. Существующий owner-loop опрашивает его на паузе и при
ожидании медленных/последнего кадров.

Ящик не владеет runner и сам не узнаёт о возврате синхронной функции. При любом
выходе (успех, ранняя ошибка источника, закрытие окна, отмена) хост обязан выставить
свой признак завершения контроллеру и дождаться его до release. Иначе worker,
ожидающий первый READY после ошибки импорта, может ждать бесконечно. Сохранённый
снимок — история наблюдения, а не доказательство, что прежняя VM ещё существует.

Live-тесты интерпретатора/JIT используют настоящий OS-поток контроллера, источники
FILE/MEMORY/HTTP, GC/запросы только у владельца, подтверждённые шаги, продолжение/
паузу, остановку медленного фильма без таймеров, STOP до startup I/O, отмену после
начала тела стартового ответа и дочернего запроса в async/blocking режимах,
очистку после runtime/ранней ошибки импорта. Без provider явно опускаются только
сетевые подслучаи. Нативная приёмка других ОС, физических устройств, race-detector/
endurance и полного отладчика остаётся открытой; стадии плана не закрываются.

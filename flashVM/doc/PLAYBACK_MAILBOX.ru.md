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

[fvm_desktop_playback_mailbox_active](../frontend/common/playback_mailbox.h#L59)()` возвращает число живых native-объектов без
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

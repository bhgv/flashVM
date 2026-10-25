# Наблюдение загрузок на основном потоке (v0.686)

[English](LOAD_MONITOR.md) · [Фоновые задания](BACKGROUND_TRANSFER.ru.md) ·
[Пояснённый пример](../examples/player/monitored_loading/README.ru.md).

[fvm_desktop_load_monitor_init](../frontend/common/load_monitor.h#L49) инициализирует [FVM_DesktopLoadMonitor](../frontend/common/load_monitor.h#L39): async child/form,
без дедлайна и callback. [fvm_desktop_run_source_monitored](../frontend/common/load_monitor.h#L57) принимает прежние options,
source, background loader и эту новую политику. Старые структуры/API сохранены.
asynchronous: 0 или 1; timeout_ms: 0 (без лимита) либо до [FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS](../frontend/common/transfer.h#L13).
Проверка предшествует notify/worker/device. Статусы: 0 успех/остановка, 1 playback error,
2 неверный ввод.

Каждый сетевой запрос получает ID внутри source-run, вид ROOT/CHILD/VARIABLES
и последовательность BEGIN, ноль или больше WAIT, один FETCHED/FAILED/CANCELLED/TIMED_OUT.
Локальные файлы и отброшенная ещё в очереди работа не наблюдаются. ROOT включает
старт и полную root-замену. Встроенный буфер не создаёт сетевого события; сетевые
потомки создают. FIFO/бюджет загрузок общие.

[FVM_DesktopLoadEvent](../frontend/common/load_monitor.h#L21) заимствован только на время notify. reference/base — копии
запроса, location ненулевой только для FETCHED. received — байты тела провайдера,
не total/wire size. BEGIN/WAIT дают NOT_FOUND. FETCHED означает полный успех fetch/sink
до импорта VM: ошибки импорта/скрипта ещё могут завершить playback без второго
transport события. VM-указателей нет. Это уведомления C-хосту, не события Flash
MovieClipLoader или script lifecycle.

[FVM_DesktopLoadNotify](../frontend/common/load_monitor.h#L37) выполняется на потоке runner вне VM callbacks, включая
старт до появления SDL окна. Разрешён собственный progress UI хоста. Нельзя входить
в VM, рекурсивно запускать runner, уничтожать SDK/provider state, бросать исключения
через C или перехватывать SDL события активного окна плеера. WAIT зависит от работы
owner loop, без real-time гарантий. Ненулевой возврат в BEGIN/WAIT успешно останавливает
весь source, отбрасывает частичные байты, join до возврата runner. BEGIN stop не запускает
провайдер. Возвраты terminal notify игнорируются. CANCELLED означает отказ от ресурса:
join во время callback ещё может ожидаться. Владение строками/job к observer не переходит.

Общий per-job deadline работает для старта, root-замены и child/form сети через timed
jobs. Timeout/provider failure сохраняют строгую ошибку playback. Close/frame bound/error
отменяют активный запрос, join предшествует освобождению target pins. DNS/несотрудничающий
код могут задержать возврат. Старым API новые callback/дедлайн автоматически не назначаются.

Тесты: настоящий HTTP и проверяемый HTTPS, разделение owner/worker, stop до/после
байтов на старте/в child, timeout,404, короткий frame bound, waiting/async успех,
игнорирование terminal stop, неверная политика до запуска. C++98-хост линкует API;
пример использует самостоятельные SWF. Полная Flash loader семантика и foreign
runtime/device acceptance остаются открытыми.

## Адаптеры playlist и консоли (v0.686)

[fvm_desktop_run_monitored](../frontend/common/load_monitor.h#L64)(options, monitor, error)` выбирает штатный worker-провайдер
для playlist. ID начинаются заново для каждого source; stop завершает весь playlist.
Без сетевого провайдера локальные файлы воспроизводятся, URL не поддерживается.
Source API сохраняет явное подключение собственного провайдера.

```sh
./build-monitor/frontend/flashvm_player --async-loads --loads 8 --load-timeout 5000 --load-progress --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

`--load-timeout MS`: строгий десятичный integer 0..2147483647, ноль отключает
дополнительный общий бюджет задания. Знаки, пробелы, дроби, exponent, overflow,
повторы и пропущенные значения отклоняются до VM/display/I/O. `--load-progress`
печатает coalesced metadata в stderr. Оба требуют `--async-loads` либо
`--background-loads`; после `--` это буквальные имена файлов. Scalar options layout
не меняется: CLI переводит флаги в C monitor. Старые API не получают неявный timeout;
I/O дедлайны провайдера остаются отдельными.

Строки содержат id/kind/phase/received/result. Фазы 1..6: BEGIN, WAIT, FETCHED,
FAILED, CANCELLED, TIMED_OUT; виды 1..3: ROOT, CHILD, VARIABLES. Result — знаковый
[FVM_Result](../include/flashvm/flashvm.h#L42) (NOT_FOUND для BEGIN/WAIT). WAIT с прежним received повторно не печатается.
Структурированные строки не содержат URL/тело ответа; обычные player diagnostics
по-прежнему могут называть URL фильма. Help/version работают без устройств, включая
корректные monitor options. Отдельной реализации сети/планировщика в CLI нет.

Owner проверяет [fvm_desktop_transfer_stop_requested](../frontend/common/transfer.h#L72), даже если job ещё PENDING.
После дедлайна новые VM turns прекращаются, затем выполняется join: медленное
подтверждение отмены провайдером больше не продлевает исполнение скрипта. Публичный
view остаётся PENDING до выхода worker, изменяемые байты не публикуются. Уведомление
TIMED_OUT, как CANCELLED, может предшествовать join. Getter возвращает 0 для none/null,
иначе CANCELLED/TIMED_OUT с первой зафиксированной причиной. Semaphore-тест подтверждает
видимость stop при ещё pending публикации worker.

## Проверка этой серии

Прошли 95 выбранных нативных проверок, затем 17 целевых после исправления stop latch.
Shared hosts: 20 native-TLS, 16 libcurl и 3 без провайдера. C-код player/transfer прошёл
UBSan на реальных monitored HTTP, CLI и job fixtures; SDL/ядро VM/криптобиблиотеки
не инструментированы. Слинкованы Windows DLL и C/C++98 consumers/monitor fixtures.
Player и пояснённый monitor example cross-link для Linux ARM64/RISC-V64 и Windows
x86_64, без запуска чужих бинарников. Source links, комментарии/API tables примера
и сортировка каталога проверены. Полная Flash conformance, physical device/foreign
runtime и все этапы остаются открытыми: stage9 release acceptance не заявляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->

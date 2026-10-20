# Наблюдение загрузок на основном потоке (v0.685)

[English](LOAD_MONITOR.md) · [Фоновые задания](BACKGROUND_TRANSFER.ru.md) ·
[Пояснённый пример](../examples/player/monitored_loading/README.ru.md).

[fvm_desktop_load_monitor_init](../frontend/common/load_monitor.h#L47) инициализирует [FVM_DesktopLoadMonitor](../frontend/common/load_monitor.h#L37): async child/form,
без дедлайна и callback. [fvm_desktop_run_source_monitored](../frontend/common/load_monitor.h#L55) принимает прежние options,
source, background loader и эту новую политику. Старые структуры/API сохранены.
asynchronous:0 или1; timeout_ms:0 (без лимита) либо до [FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS](../frontend/common/transfer.h#L13).
Проверка предшествует notify/worker/device. Статусы:0 успех/остановка,1 playback error,
2 неверный ввод.

Каждый сетевой запрос получает ID внутри source-run, вид ROOT/CHILD/VARIABLES
и последовательность BEGIN, ноль или больше WAIT, один FETCHED/FAILED/CANCELLED/TIMED_OUT.
Локальные файлы и отброшенная ещё в очереди работа не наблюдаются. ROOT включает
старт и полную root-замену. Встроенный буфер не создаёт сетевого события; сетевые
потомки создают. FIFO/бюджет загрузок общие.

[FVM_DesktopLoadEvent](../frontend/common/load_monitor.h#L20) заимствован только на время notify. reference/base — копии
запроса, location ненулевой только для FETCHED. received — байты тела провайдера,
не total/wire size. BEGIN/WAIT дают NOT_FOUND. FETCHED означает полный успех fetch/sink
до импорта VM: ошибки импорта/скрипта ещё могут завершить playback без второго
transport события. VM-указателей нет. Это уведомления C-хосту, не события Flash
MovieClipLoader или script lifecycle.

[FVM_DesktopLoadNotify](../frontend/common/load_monitor.h#L35) выполняется на потоке runner вне VM callbacks, включая
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

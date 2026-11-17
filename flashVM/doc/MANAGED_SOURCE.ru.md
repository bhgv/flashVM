# Управляемая загрузка movie — v0.1703

[English](MANAGED_SOURCE.md) · [Принятая модель DisVM](../../disVM/runtime/FLASHVM-IO-MODES.md)

Общий player получил отдельный управляемый вход [`fvm_desktop_run_source_managed`](../frontend/common/managed_source.h#L48).
Начальный SWF, замена root, вложенные movie и данные `loadVariables` получают
байты только через явно заданный native bridge. Обычный [`fvm_desktop_run_source`](../frontend/common/source.h#L75)
сохраняет текущую самостоятельную файловую/HTTP-загрузку.

Реализована сторона player. Кодирование запросов моста в Styx/S-record и сервер
DisVM ещё не подключены. Новые socket/write API, media transport, автоматическое
создание дополнительных каналов и фильтрация ввода этой версией не добавляются.

## Интерфейс и владение

| API/type | Контракт |
| --- | --- |
| [`FVM_DesktopManagedLoader`](../frontend/common/managed_source.h#L28) | Точный `struct_size`, обязательный `fetch`, borrowed `userdata`, общий бюджет `max_loads` |
| `fvm_desktop_managed_loader_init(out)` | Инициализация без I/O:1 для output,0 для NULL; bridge отсутствует, начальный бюджет1 |
| [`FVM_DesktopManagedFetch`](../frontend/common/managed_source.h#L21) | Синхронный native callback: purpose, исходное имя/base, byte sink, буфер canonical location и host state |
| `fvm_desktop_run_source_managed(options, source, loader, error)` | Копирует policy; выполняет обычный owner-thread run; статусы desktop0/1/2 и optional static error |

Caller удерживает options, source bytes и transport state до возврата. Callback
не входит в VM/SDL, не разрушает player, не вызывает вложенный playback и не
сохраняет borrowed inputs/sink. Native callback/userdata pointers не передаются
по сети. Новых concurrent/reentrant player runs нет. Как прежний synchronous
fetch, callback может блокироваться; asynchronous managed runner ещё предстоит.

Bridge разрешает reference относительно movie base в выданной VFS, возвращает
байты произвольными chunks и ограниченное, непустое, NUL-terminated canonical
имя VFS. На отказ sink нужно остановиться и вернуть реальный status. Сохраняется
существующий предел размера movie. При любом отказе staged bytes освобождаются
до import. Повторного поиска в локальных файлах или HTTP после отказа нет.

## Назначение и origin

[`FVM_DESKTOP_FETCH_CODE`](../frontend/common/managed_source.h#L8) передаётся для начальной/root/вложенной загрузки movie.
Мост DisVM должен использовать provider-first `DVM_RESOURCE_SWF`.
[`FVM_DESKTOP_FETCH_DATA`](../frontend/common/managed_source.h#L9) передаётся для variable/form данных и соответствует
обычному `DVM_RESOURCE_DATA`. Это native identifiers, а не новые wire tags или
права доступа. Movie-load может принимать поддерживаемое изображение; операция
при этом остаётся execution-capable и классифицируется CODE. `loadVariables`
всегда DATA, даже с именем `.swf`; расширение имени не определяет политику.

FILE и URL идут через bridge одинаково. MEMORY SWF уже передан владельцем моста;
его location сохраняется как явный VFS-origin, в том числе относительный. Каталог
исполняемого файла или CWD не подставляется. Вложенный movie получает собственный
canonical base для последующих относительных запросов.

Полномочия определяет мост/DisVM VFS. В управляемой загрузке не вызываются native
file reader и compiled HTTP-policy adapter. Проверки формы AVM1-запроса и формата
movie сохраняются. Текущий loader поддерживает GET/no-method; неподдержанные
POST/browser-navigation явно отклоняются. Другие embeddings и trusted host
extensions должны установить соответствующий маршрут; process-wide OS sandbox
этим API не вводится. Локальный ввод player сохраняется.

## Исполняемые проверки

Source-player test выполняет настоящие SWF в interpreter/JIT: root создаёт child,
загружает его SWF и переменные, child загружает данные относительно своего origin.
Скрипт проверяет значения. Bridge выдаёт bytes по3, тестируются FILE/URL/MEMORY,
CODE для `.data` и DATA для `.swf`. Отложенная замена root сохраняет мост
после освобождения старого VM-контекста, задаёт новый canonical base для
последующих загрузок и соблюдает общий лимит acquisitions.

Во время тестов отказа существует валидный локальный movie. NOT_FOUND, частичный
ответ с ошибкой, превышение sink limit и повреждённый canonical origin не приводят
к его загрузке. Invalid descriptor/missing bridge отклоняются до acquisition.

Большой player разделён на bounded source units; до изменения поведения проверено
совпадение препроцессированных native tokens. Модель owner/GC не менялась.
Команды сборки и запуска приведены в [English guide](MANAGED_SOURCE.md#validation).
Нужны SDL2 SDK и C89/C++98 toolchain. SDL dummy-video не означает приёмку на
физическом дисплее или устройствах ввода; стадии полной SWF-совместимости открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_DESKTOP_FETCH_CODE` | macro / define | [frontend/common/managed_source.h:8](../frontend/common/managed_source.h#L8) |
| `FVM_DESKTOP_FETCH_DATA` | macro / define | [frontend/common/managed_source.h:9](../frontend/common/managed_source.h#L9) |
| `fvm_desktop_managed_loader_init` | function | [frontend/common/managed_source.h:39](../frontend/common/managed_source.h#L39) |
| `fvm_desktop_run_source` | function | [frontend/common/source.h:75](../frontend/common/source.h#L75) |
| `fvm_desktop_run_source_managed` | function | [frontend/common/managed_source.h:48](../frontend/common/managed_source.h#L48) |
| `FVM_DesktopManagedFetch` | callback typedef | [frontend/common/managed_source.h:21](../frontend/common/managed_source.h#L21) |
| `FVM_DesktopManagedLoader` | struct | [frontend/common/managed_source.h:28](../frontend/common/managed_source.h#L28) |
<!-- source-links:end -->

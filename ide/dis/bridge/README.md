[F7 Build / F8 diagnostics — v0.1671](../../BUILD-PANEL.md) · [Русский](../../BUILD-PANEL.ru.md)

# Прикладные сервисы редактора на Limbo

[Explicit values and references](../VALUES.md)

[Projection owner and watch-row model](../PROJECTIONS.md)

[Архитектура RU](../../EDITOR-CHANNELS.ru.md) · [Architecture EN](../../EDITOR-CHANNELS.md) ·
[AS view/worker](../../as/README.md) · [Owner pump](../../../disVM/OWNER-PUMP.ru.md)

## Модули и владельцы

| Модуль | Работа | Владение и каналы |
| --- | --- | --- |
| Idecore | Документ, ревизии, поиск, история | Собственный процесс, `chan of Request` |
| Uibridge | Общий value-only кодек | Чистые функции; массивы/строки принадлежат Dis |
| Ideinput | Курсор, выделение, клавиатурные команды | Один State у контроллера; typed calls к документу |
| Idebridge | Протокол и композиция editor owner | Обычные byte-array каналы input/output |
| Idecomplete | Индекс имён/сигнатур | Отдельный процесс и поколение данных |
| Idebuiltins | AS/Limbo встроенные декларации и параметры | Независимые value-records для публикации/справки |

`serveapp` запускает документный owner и применяет ввод через Ideinput.
`serve` вместо этого передаёт Pointer/Key/Text/Focus в предоставленный канал
другого Dis-сервиса. Во втором варианте приложение обязано принимать эти события:
их отправка может приостановить процесс bridge по обычной семантике Limbo.
Ни один из продуктовых модулей не импортирует Sys, Draw, Tk или нативные окна.

## Команды и снимки

Явный запрос использует общий Request envelope: `code=Idecore operation`,
`flags=expected revision`, `first/second=диапазон или координаты`, `text=аргумент`.
ID редакторского запроса должен быть положительным int32. Доступные номера
операций берутся из idecore.m; новый парсер программ здесь не пишется.

Для ReplaceAll text содержит восемь lowercase hex-цифр длины needle в UTF-8 байтах,
затем needle и replacement. NUL не является разделителем и сохраняется в строках.
Срез по середине Unicode запрещён проверкой точного обратного преобразования.

Reply повторяет ID и передаёт `code=status`, `flags=revision`, `first=opened`,
`second=saved revision`. Его payload начинается с семи восьмизначных hex-полей:
`offset`, `line`, `column`, `matches`, `selection_end`, `byte_offset`, `total_bytes`.
Первые пять — int32 с сохранением -1, последние два — неотрицательные byte extents.
За 56 ASCII-байтами метаданных идёт до 968 байт UTF-8 снимка. Между фрагментами
не меняются ID, статус, ревизия, saved/opened и координаты результата.

Ввод публикует такие же снимки классом View=7 с id=0. Дополнительная View-запись
с `code=65536`, `flags=revision`, `first=caret`, `second=anchor`, пустым text
передаёт выбор, уже решённый Dis. AS только отображает эти Unicode-координаты.
После успешного Shutdown дополнительной selection-записи нет: финальный ответ
можно принять и завершить вызывающую сторону без зависания на скрытом сообщении.

Протокольная ошибка завершает частный документный owner до возврата -1.
Это не полная система отмены/таймаутов: внешняя остановка процессов по-прежнему
требует координации хоста. Максимальный request payload остаётся 1024 байта;
длинные документы могут расти серией правок, но потоковое открытие больших файлов
пока не реализовано. Reply fragmentation не следует путать с готовым file protocol.

## Ввод и completion

Текстовый ввод поступает классом Text. Key не превращается дополнительно в ASCII
текст, иначе одна клавиша могла бы вставить символ дважды. Поддержаны Left/Right,
Home/End, Shift-выбор, Backspace/Delete, Ctrl+A/Z/Y. Focus loss сбрасывает Control
и Shift. Pointer, вертикальная геометрия строк, IME composition и popup-navigation
ещё не реализованы как законченная редакторская политика.

Idecomplete принимает уже полученные записи деклараций, а не парсит программный
текст. Publish требует новое положительное поколение и атомарно копирует/сортирует
пакет. Дубликат name/kind/signature отвергает весь пакет. Query требует точное
поколение, case-sensitive prefix и limit 1..64. Индекс ограничен 4096 элементами,
каждое текстовое поле — 512 UTF-8 байтами, параметров в сигнатуре не больше 16,
совокупный текст поколения не больше 4 MiB. Query возвращает независимые копии.
Idebuiltins предоставляет 135 AS и 64 Limbo элемента: keywords/operators, системные
объекты, методы, их параметры, короткую справку и overload-aware active argument.
Это начальная документированная выборка, не полный Flash Player API. Для AS
учтено ограничение uniform setTextFormat; Sys описывает интерфейс модуля, не
готовую реализацию Sys в нашем хосте.

Публикация имеет O(n log n) сортировку; поиск — O(log n + limit) сравнений строк.
Фактические адаптеры compiler/module-provider и окончательная вставка выбранной
подсказки ещё не подключены к приложению.

## Сборка и проверки

Из корня репозитория: `./build-workspace.sh --projects ide` собирает модули и `.sbl`
без обязательного emu. Standalone CMake получает явные SDK:

```sh
cmake -Hide/dis -Bbuild/ide \
    -DLIMBO_EXECUTABLE=/absolute/path/to/source-built-limbo \
    -DUIBRIDGE_SOURCE_ROOT="$PWD/disVM"
cmake --build build/ide -- -j2
```

Для reference-тестов дополнительно задаются `LIMBO_MODULE_DIR` и `EMU_EXECUTABLE`
от **совместимого** набора compiler/header/runtime. С этими параметрами собираются
idebridge_test и idecomplete_test, регистрируются режимы 0/1. Компиляция нашим
bootstrap также проверена, но это не свидетельство совместимости его ABI с любым
эмулятором. Необязательные AS проверки используют явные IDE_AS_SOURCE_ROOT,
AS2_PARSE_EXECUTABLE и NODE_EXECUTABLE. Они не нужны исполняемой IDE.

Приложение прошло reference-проверку реальных каналов и 36 декодированных кадров,
включая Unicode, историю и разрезанный у границы UTF-8 снимок. Индекс прошёл
проверки поколений, префиксов и alias isolation. Это не запуск всего приложения
в незавершённом embedded Dis host и не доказательство native JIT для всех платформ.

## English

The modules keep editor policy in Dis and rendering near AS. Typed calls drive
real document/history owners; ordinary byte-array channels carry correlated
commands, bounded snapshot fragments and unsolicited View/selection values.
The completion owner consumes declaration records with atomic generations,
merge-sort publication, binary-search prefixes and copied bounded results.
Providers, popup insertion, pointer hit mapping, streaming file opening and the
complete embedded host remain unfinished. Root builds compile modules/debug data;
optional reference execution requires an explicitly matching compiler/header/VM.

[Dis workspace and compiler-job ownership / Рабочие документы и задания сборки](../../WORKSPACE.md).

## Dynamic object direction / Направление динамических объектов

[Channel-owned any/object proxies](../../../fir/v1/doc/DYNAMIC_VALUES.md) keep
representation and conversion in the owning VM. This is the selected design,
not an implemented IDE bridge operation or source any lowering.

[Native compiler jobs / Библиотечные сборки на Dis](../../COMPILER-SERVICE.md) · [Русский](../../COMPILER-SERVICE.ru.md): actual Coco/R/FIR/SWL libraries, revision-bound workspace worker, copied diagnostics and artifacts. F7 Build/F8 diagnostics now use the portable revision-bound panel; Run/Debug remain future work.

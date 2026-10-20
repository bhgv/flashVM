# Источники SWF и встраивание

[English](PLAYER_SOURCES.md).

Текущая v0.686 поддерживает дочерние фильмы и loadVariables; она заменяет описанные ниже исторические ограничения root-only.

[Встроенный транспорт POSIX/Winsock без libcurl](SOCKET_TRANSPORT.ru.md) доступен с v0.677.

## Вход из памяти с v0.673

[fvm_desktop_run_memory](../frontend/common/player.h#L44) из [player.h](../frontend/common/player.h) принимает
скалярные [FVM_DesktopOptions](../frontend/common/load_monitor.h#L57), const void указатель, точное число байтов size_t,
непустое имя источника и необязательный указатель на статическую ошибку.
Подходят char buf[] и read-only массивы в исполняемом файле. NUL — обычный байт,
не конец данных. Нет временного файла, strlen(data), изменения источника или
передачи владения. Сохраняйте байты и имя доступными и неизменными до синхронного
возврата. Импорт VM владеет своей копией. FWS и CWS используют те же проверки,
сцену, корни GC, планировщик, рендеринг и очистку, что и файловый вход.

Поля плейлиста игнорируются, скалярные параметры проверяются полностью.
Null, нулевой/чрезмерный размер, отсутствующее имя и неверные параметры дают
код 2 до создания VM/окна. Повреждённый SWF даёт код 1 до открытия окна.
Успех/закрытие возвращает 0. Error получает null либо статический текст,
освобождать его нельзя. Прочая диагностика пока использует стандартные потоки.
Лимит входа [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) — 64 MiB; лимиты распакованного SWF сохраняются.

Вызов выполняется на основном UI/VM-потоке, без параллельного или рекурсивного
воспроизведения. Это синхронный проигрыватель, не widget API. Имя — метка и
происхождение источника, а не открываемый путь. Финальный SDL_Quit принадлежит
приложению. [Владение](PLAYER_HOST_OWNERSHIP.ru.md).

[Пример встроенного фильма](../examples/player/embedded_movie/README.ru.md)
линкует неизменный SWF в программу и не требует файла ресурса при запуске.
Регрессии C89 проигрывают stack char и CWS в interpreter/JIT, проверяют ошибки,
неизменность байтов и используют несуществующие пути источников.

```sh
cmake --build build-player --target flashvm_example_embedded_movie flashvm_frontend_memory_movie
./build-player/frontend/flashvm_example_embedded_movie
ctest --test-dir build-player -R 'player_memory|example_embedded' --output-on-failure
```

[Нативная/кросс-сборка](DESKTOP_FRONTEND.ru.md) ·
[Параметры хоста](PLAYER_CONFIGURATION.ru.md) · [Реализация](../frontend/common/player.c) ·
[Этапы](ROADMAP_STATUS.md)
## Транспорт хоста и корневая загрузка с v0.674

[source.h](../frontend/common/source.h) содержит [FVM_DesktopSource](../frontend/common/load_monitor.h#L58) (kind, bytes,
size, location), [FVM_DesktopLoader](../frontend/common/source.h#L44) (fetch, userdata, max_loads), [FVM_DesktopWrite](../frontend/common/transfer.h#L25)
и [FVM_DesktopFetch](../frontend/common/http.h#L17). [fvm_desktop_loader_init](../frontend/common/source.h#L51) задаёт отсутствие транспорта и
max_loads=1. [fvm_desktop_run_source](../frontend/common/source.h#L60) принимает MEMORY, FILE или URL; проверки
скалярных параметров, игнорирование плейлиста, коды/ошибки и UI-поток совпадают
с memory run. Для FILE/URL bytes=null и size=0. Имя — 1..4096 байтов;
max_loads — 1..1024 с учётом стартового фильма. Структуры и данные заимствуются
до синхронного возврата. FILE использует Unicode-адаптер SDL. URL без fetch
явно завершается ошибкой; сетевые зависимости в ядро VM не добавлены.

Fetch получает ссылку, необязательный предыдущий canonical base, write и его
userdata, выходной буфер canonical location/его вместимость и userdata хоста.
Разрешите относительную ссылку, передавайте двоичные части синхронно, запишите
непустую абсолютную effective location с NUL в пределах capacity. Она станет
базой следующего фильма. Нельзя сохранять указатели, вызывать VM/SDL, рекурсивно
запускать плеер или выбрасывать C++ исключения через границу C. Возвращайте
фактическую ошибку транспорта; при ошибке write немедленно остановитесь.
Sink ограничивает накопление 64 MiB, проверяет арифметику и запоминает первую
ошибку. Пустой/частичный ответ не проигрывается даже при игнорировании ошибки
провайдером. Сеансы, авторизация, отмена и планирование принадлежат хосту;
синхронный callback может блокировать UI.

При max_loads>1 устанавливается существующий rooted URL sink AVM1.
GetURL2 LOAD_TARGET к _root или _level0 с GET/без метода копирует counted reference
в native-память. Текущий callback/кадр может завершиться; сеть и рекурсивный
запуск внутри него не выполняются. На безопасной границе старые VM/аудио/окно
освобождаются, затем загружается новый фильм с новой VM. Управляемые указатели
не переживают замену. Допускается один ожидающий запрос; второй неподдержан.
Лимит общего числа фильмов останавливает циклы кодом 1; закрытие завершает цепь.
Ошибка последующей загрузки не восстанавливает закрытый фильм. Дочерние клипы,
прочие уровни, POST, loadVariables и browser navigation не поддержаны. Это
ограниченная корневая замена, не полный loadMovie Flash и не progressive loading.

В FILE-цепи относительный путь присоединяется к каталогу предыдущего файла;
абсолютный сохраняется. URL разрешает провайдер. Фильм в памяти может передать
своё имя как base провайдеру; абсолютный запрос от него не зависит. Транспорт
вызывается на основном потоке только после освобождения прежней VM.
[source_movie.c](../frontend/tests/source_movie.c) проверяет части/base,
частичный отказ, игнорирование overflow, неверный canonical output, _root/_level0,
неподдержанные запросы и лимиты циклов в interpreter и JIT.
## HTTP(S), CLI и библиотечные модули с v0.675

FVM_PLAYER_HTTP=ON подключает отдельный SDK libcurl 7.62+ только к необязательному
frontend. По умолчанию OFF: память, файлы и пользовательский транспорт остаются
доступны без curl. [fvm_desktop_http_fetch](../frontend/common/http.h#L22) из [http.h](../frontend/common/http.h)
соответствует [FVM_DesktopFetch](../frontend/common/http.h#L17): присвойте loader.fetch, userdata оставьте null.
URL API curl разрешает reference/base и возвращает canonical provenance.
HTTP(S) GET проверяет TLS peer/hostname, полное завершение передачи и код 2xx.
Редиректы отклоняются. POST, настройка авторизации, progressive loading и отмена
не добавлены. Таймаут подключения/всей передачи — 10/30 секунд. Используются
системное доверие сертификатам и настройки proxy curl. Вызов балансирует свою
curl-global ссылку и освобождает собственные handles. Хост с собственным curl
сохраняет свою global-ссылку и согласует инициализацию/очистку, особенно для
старых SDK без потокобезопасной global initialization.

CLI и структурированный плейлист распознают URI по ://. При HTTP-сборке
используется этот провайдер; без транспорта URI явно завершается ошибкой.
--loads N задаёт бюджет корневых загрузок для каждого стартового фильма,
включая сам старт: 1..1024, по умолчанию 1. Поле [FVM_DesktopOptions.max_loads](../frontend/common/options.h#L35)
задаёт ту же политику CLI/плейлиста; прямой run_source использует явный
[FVM_DesktopLoader](../frontend/common/source.h#L44). Закрытие заканчивает весь плейлист. Несколько файлов по
умолчанию играются по одному авторскому проходу. Неподдержанные targets/POST
дают ошибку, не превращаются молча в корневую навигацию.

```sh
cmake -S flashVM -B build-player -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HTTP=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/sdl/lib/cmake/SDL2
cmake --build build-player --target flashvm_player
./build-player/frontend/flashvm_player --mute --loads 16 https://example.org/start.swf
```

При встраивании C/C++ подключите player.h и http.h, инициализируйте options/loader,
задайте source.kind=FVM_DESKTOP_SOURCE_URL, bytes=null, size=0, location=URL;
loader.fetch=[fvm_desktop_http_fetch](../frontend/common/http.h#L22), затем [fvm_desktop_run_source](../frontend/common/source.h#L60). Для стартового
ресурса в программе выберите MEMORY и точный sizeof массива; тот же loader
может обслуживать запросы из встроенного фильма. Относительная ссылка разрешается
провайдером только при корректной HTTP(S)-базе источника.

FVM_PLAYER_HOST_SHARED=ON превращает flashvm_player_host в динамический модуль
и включает PIC для статических зависимостей. OFF оставляет архив. Windows
экспортирует символы хоста через CMake; это по-прежнему внутренний C89/C++98
frontend-интерфейс. Нужны целевые SDL/curl SDK с PIC для статических зависимостей
ELF/Mach-O shared library. Нельзя использовать non-PIC SDL архивы. Библиотеки
ядра остаются статическими; глобальный BUILD_SHARED_LIBS не переключается.

```sh
cmake -S flashVM -B build-module -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HOST_SHARED=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/pic-sdl/lib/cmake/SDL2
cmake --build build-module --target flashvm_example_module_host
./build-module/frontend/flashvm_example_module_host
```

[Пример ресурсного модуля](../examples/player/embedded_movie/README.ru.md) тоже
строится архивом либо динамической библиотекой при включённой опции. Он содержит
свой неизменный SWF и экспортирует embedded_movie_play для обычного C/C++-хоста;
файл фильма при запуске не нужен. Android сохраняет SDLActivity; desktop module
host там не собирается. Целевые SDK/toolchain описаны в руководстве frontend.

Loopback HTTP-тест проверяет стартовый и запрошенный SWF относительный GET,
chunked, 404, отказ редиректа, оборванный ответ и локальные относительные загрузки
в interpreter/JIT. Публичная сеть, доверенный HTTPS endpoint, физические устройства
и полная семантика загрузки Flash этой проверкой не сертифицируются.
## Дочерние клипы, переменные и рабочие базы с v0.676

FIFO принимает несколько GetURL2/global/MovieClip-запросов за кадр. Хост
устанавливает глобальные loadMovie/loadVariables и методы MovieClip. Поддержаны
GET и отсутствие метода. Target разрешается через активную display scope, включая
относительные пути; _root учитывает native root/lockroot, _level0 выбирает корень
плеера. loadVariables может адресовать и обычный объект. Запрос копирует URL и
базу исходного фильма, удерживает resolved target до завершения. Сеть/файл не
открываются внутри активного script callback. Дочерние ресурсы и переменные
доставляются на следующей безопасной границе основного потока без удаления
родительской VM. Полная root-замена по-прежнему удаляет старую VM и безопасно
отменяет оставшиеся её запросы. Ошибка/закрытие освобождает все queued roots.
--loads и max_loads теперь считают старт и все movie/form запросы.

Для встроенного старта база по умолчанию — каталог исполняемого файла, никогда
не process CWD. Linux/Windows используют SDL_GetBasePath; macOS — dyld и realpath
(каталог executable, а не Resources приложения); Android — /proc/self/exe.
APK-хост может явно задать абсолютное имя источника с другим каталогом ресурсов.
Ветки обнаружения macOS/Android ещё требуют проверки с настоящими SDK и запуском. Полное filesystem- или HTTP(S)-имя
источника явно задаёт provenance; это полезно для библиотеки или OS sandbox,
запрещающей обнаружение исполняемого файла. Без обнаружения/явного origin старт
работает, но относительный ресурс даёт ошибку без подмены на CWD. В этой среде
нет /proc/self/exe: тест изолирует OS discovery, моделирует каталог executable,
удаляет исходный файл старта, меняет CWD и выполняет настоящую загрузку ресурсов.
Сетевой старт использует canonical URL. Загруженный child сохраняет свой URL,
его native-потомки наследуют ближайшую source base. Поэтому loadVariables из
child разрешается относительно каталога child, а не target или стартового фильма.
Глобального chdir/сетевого рабочего каталога нет.

### C89 API ядра

[fvm_movieclip_set_source](../include/flashvm/movieclip.h#L103) копирует counted location (1..4096, без NUL) в приватный
управляемый граф клипа. Требуются same-context clip и живой основной поток;
script не вызывается. [fvm_movieclip_get_source](../include/flashvm/movieclip.h#L107) читает ближайшую native ancestor
base, возвращая заимствованную строку; NOT_FOUND/ошибки сохраняют out. Удерживайте
клип через GC. Приватные метаданные входят в обычный граф; callback-код хоста
и транспортные сеансы не сериализуются.

[fvm_movieclip_replace_swf](../include/flashvm/movieclip.h#L114) импортирует в тот же контекст, сохраняя объект target,
родительский depth slot, transform/shear и существующие host/script own properties.
Удаляются только именные bindings прежних display children; прочие сохранённые
ссылки остаются. Старые children отделяются. Root новой библиотеки указывает на
существующий target, новый source сохраняется. Frame actions ждут следующего
advance. Все возможные отказы staging предшествуют публикации: повреждённый
вход/OOM сохраняет прежнее содержимое. Только owner thread вне VM/native callback,
GC/teardown; вложенная замена даёт INVALID_ARGUMENT. Строгий текущий importer
отклоняет неподдержанные теги и root streaming sound дочернего фильма. Успех
запрашивает перерисовку.

[fvm_object_load_variables](../include/flashvm/movieclip.h#L155) принимает same-context target и counted form до 64 MiB.
Сначала декодируются все percent escapes/plus/пробелы и WTF-8, до 65536 полей, включая игнорируемые пустые сегменты. NUL в декодированных ключах/значениях остаётся counted. Повторные
ключи присваиваются по порядку, последний выигрывает; без equals значение пустое,
пустой ключ игнорируется. Ошибка декодирования не меняет свойства. Публичные
сеттеры могут выполнять script: при отказе ранее совершённые эффекты сохраняются.
После присваивания вызывается необязательный onData; его ошибка также не откатывает
данные. Пустой вход успешно завершает операцию и вызывает handler. Native collector
barrier защищает target/временные данные; вход заимствуется до возврата. Успех
запрашивает redraw. В этом API нет транспорта и конвертации старых non-UTF-8 code pages.

Фасады поддерживают обычные global/method формы аргументов. AVM1 может найти
unqualified имя через clip prototype до global fallback: форма выбирается по
target против GET/POST аргументов. Полная историческая lookup/shadowing/coercion
совместимость остаётся открытой. POST, browser navigation, progressive/cancellable
транспорт, полный набор MovieClipLoader events и независимые child sound dictionaries (event/stream) не поддержаны.
Доступны libcurl, провайдер приложения и встроенный сокетный backend: [POSIX/Winsock HTTP(S)](SOCKET_TRANSPORT.ru.md).

[Исполняемый пример](../examples/player/loading/README.ru.md) ·
[API ядра](../include/flashvm/movieclip.h) ·
[Реализация](../src/standard/movieclip_loading.cpp) ·
[Тесты ядра](../tests/movieclip_loading.cpp) ·
[Перебор отказов выделения](../tests/movieclip_loading_allocation.cpp) ·
[HTTP/embedded тесты](../frontend/tests/child_loading.py)
### Непрозрачное завершение сеанса и экспорт Windows

[fvm_desktop_shutdown](../frontend/common/player.h#L51) из player.h явно завершает SDL именно этой библиотеки на
её UI-потоке после возврата playback. Если SDL_WasInit видит активную подсистему,
возвращается [FVM_ERROR](../include/flashvm/flashvm.h#L43) без изменений; иначе вызывается SDL_Quit и возвращается
[FVM_OK](../include/flashvm/flashvm.h#L42), в том числе при повторном завершении неиспользуемого сеанса. Операция
может очистить hints/TLS: приложение вызывает её только в конце своего сеанса.
Автоматический глобальный quit в закрытие окна/VM не добавлен.
Это работает и для DLL с приватным статическим SDL: обычному приложению не надо
линковать/вызывать вторую копию SDL. Модуль-пример экспортирует
embedded_movie_shutdown рядом с embedded_movie_play; module_host.c включает
только module.h. Нельзя выгружать модуль во время активного запуска.

Декларации Windows DLL используют FVM_DESKTOP_API из export.h с явными
 dllexport/dllimport. CMake задаёт FVM_DESKTOP_SHARED публично и FVM_DESKTOP_BUILD
приватно для shared host. При ручной линковке import library потребитель задаёт
FVM_DESKTOP_SHARED; для статического архива не задаёт. ELF/Mach-O сохраняют
обычную visibility. Linux RPATH плеера ищет библиотеку рядом и в lib/.
Shared native CTest явно добавляет build-каталог библиотек, если sandbox без
/proc мешает $ORIGIN. Защита активных владельцев проверяется в static-host
фикстуре с общим SDL; opaque DLL cleanup проверяется через интерфейс модуля.
GET в этой серии — получение ресурса по заданному URL. Автоматическая отправка переменных текущего клипа в query и POST-body ещё не реализована.
[Controlled/background C APIs / Управляемая фоновая загрузка](BACKGROUND_TRANSFER.ru.md), v0.680–0.683.
[Наблюдение загрузок, дедлайны и отмена старта](LOAD_MONITOR.ru.md).
[Native content unloading / Очистка содержимого](MOVIE_UNLOAD.ru.md).
[Linked-memory startup with child unload/reload / Встроенный старт и очистка](../examples/player/unload_loading/README.ru.md).
[Standalone image replacement / Изображения в клипе](IMAGE_LOADING.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_movieclip_get_source` | function | [include/flashvm/movieclip.h:107](../include/flashvm/movieclip.h#L107) |
| `fvm_movieclip_replace_swf` | function | [include/flashvm/movieclip.h:114](../include/flashvm/movieclip.h#L114) |
| `fvm_movieclip_set_source` | function | [include/flashvm/movieclip.h:103](../include/flashvm/movieclip.h#L103) |
| `fvm_object_load_variables` | function | [include/flashvm/movieclip.h:155](../include/flashvm/movieclip.h#L155) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
<!-- source-links:end -->

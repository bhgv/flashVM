Версия разработки: **0.603** ([VERSION.txt](VERSION.txt)). Все стадии
[роудмапа](doc/ROADMAP.md) остаются открытыми; текущие доказательства и
ограничения описаны в [приёмке](doc/RELEASE_ACCEPTANCE.md).
По действующему поручению основным временно является [GitHub](https://github.com/bhgv/flashVM):
новые коммиты отправляются только туда, с проверкой серверного HEAD.
Флаг `--jit-emulator` записывает новые проверки инструкций/ABI в Unicorn;
[условия проверки](doc/PLATFORM_VALIDATION.ru.md).

v0.581: API очереди PCM с повтором записи, часы потребления и настоящий
SDL2-вывод при явном включении SDK. [Руководство](doc/SWF_SOUND_OUTPUT.ru.md) /
[EN](doc/SWF_SOUND_OUTPUT.md), [учебный пример C89](examples/multimedia/swf_audio_queue/README.md).
Часы программной FIFO, не ЦАП; автоматическая desktop A/V синхронизация ещё открыта.

Тестовые плееры v0.582: [Linux x64, Windows x64, Android ARM64](doc/PLAYER_TEST_BUILDS.ru.md)
и [три готовых SWF](examples/player/animation_audio/README.md).

v0.584: [Android PCM/WAV diagnostics / диагностика звука](doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md).

v0.586: [согласование блока SDL и запаса очереди](doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md#v0586-публикация-согласованного-блока-устройства).

v0.587: [optional stop release / затухание остановки](doc/PLAYER_AUDIO_RELEASE.md).

v0.588: [ADPCM test-tone block restart fix / исправление тестового ADPCM-тона](doc/PLAYER_AUDIO_DIAGNOSTICS.md#v0588-adpcm-fixture-block-restarts).

v0.589: [Опрос Key и клавиатурные снимки хоста](doc/KEYBOARD.ru.md).

v0.603: [Mixed gesture/Mouse/Key FIFO and bounded cancellation / общая очередь ввода](doc/INPUT_QUEUE.md), [C89 tutorial](examples/embedding/mixed_input/README.md).

v0.602: [Separate patch for the Clutter3D library / отдельный патч библиотеки](examples/frontends/clutter3d/LIBRARY_PATCH.md).

v0.601: [Clutter fork subpixel inverse and Linux EGL/GLES2 SDK / дробные координаты и EGL/GLES2](external/clutter3d/README.md#linux-eglgles2-profile-v0601).

v0.600: [Frontend tutorials / примеры фронтендов](examples/frontends/README.md): SDL2, selected Clutter/Cogl fork, relocated headless host; [SDK source and fixes](external/clutter3d/README.md).

v0.599: [Runnable host without SDL / запускаемый хост без SDL](examples/frontends/headless/README.md); shared pointer projection in the SDL FIFO.

v0.598: [Independent frame/event host boundary / независимый хост кадров и событий](doc/HOST_BACKENDS.md) ([RU](doc/HOST_BACKENDS.ru.md)).

v0.597: [Muted sound-bearing movie playback / запуск звуковых роликов с --mute](doc/PLAYER_POINTER.md#muted-movie-playback) ([RU](doc/PLAYER_POINTER.ru.md#ролики-без-звука)).

v0.596: [Player primary touch and shared input FIFO / касания и общая очередь ввода](doc/PLAYER_POINTER.md) ([RU](doc/PLAYER_POINTER.ru.md)).

v0.595: [Primary-pointer Mouse listeners / события касания и указателя](doc/MOUSE.md) ([RU](doc/MOUSE.ru.md)).

v0.594: [Selection observers / наблюдатели смены фокуса](doc/SELECTION.md#selection-listeners) ([RU](doc/SELECTION.ru.md#слушатели-selection)).

v0.593: [MovieClip focus notifications / уведомления смены фокуса](doc/SELECTION.md#focus-notifications) ([RU](doc/SELECTION.ru.md#уведомления-фокуса)).

v0.592: [Selection MovieClip subset / скриптовый выбор клипа](doc/SELECTION.md) ([RU](doc/SELECTION.ru.md)).

v0.591: [Explicit host keyboard focus / выбор клипа для клавиатуры](doc/KEYBOARD_FOCUS.md) ([RU](doc/KEYBOARD_FOCUS.ru.md)).

v0.590: [Key listeners and ordered transitions / слушатели и переходы](doc/KEYBOARD_LISTENERS.md) ([RU](doc/KEYBOARD_LISTENERS.ru.md)).

## Просмотр документации

Из корня проекта установите зависимости и запустите локальный просмотрщик:

```sh
python -m pip install -r doku_viewer/requirements.txt
python view_docs.py
```

[Руководство просмотрщика](doku_viewer/README.md). Python 2.7.9 / Python 3.


Повторяемые AGG/Cairo/minimal и адаптер Android CTest/ADB:
[проверка платформ](doc/PLATFORM_VALIDATION.ru.md). Реальные запуски остальных
ОС и завершение стадий 1–9 пока не подтверждены.

В JIT добавлены запись/объявление переменных, свойства и удаление:
[контракты и проверки](doc/JIT_REFERENCES.ru.md).
Float/double и семь унарных операций работают в явном и автоматическом JIT:
[числовые контракты и проверки](doc/JIT_NUMBERS.ru.md).
Регистры JIT используют готовые банки Function2/контекста/корневого блока:
[контракты и проверки](doc/JIT_REGISTERS.ru.md).
ConstantPool и индексированные Push сохраняют захваченные пулы и выбор по ветвям;
[контракты и проверки](doc/JIT_POOLS.ru.md).
CallMethod/NewMethod используют общий нативный сервис с корнями GC;
[контракты и проверки](doc/JIT_METHODS.ru.md).
CallFunction/NewObject сохраняют лексический и версионный поиск имён;
[контракты и проверки](doc/JIT_NAMED_CALLS.ru.md).
SetTarget/SetTarget2 выбирают временную шкалу с восстановлением цели;
[контракты и проверки](doc/JIT_TARGETS.ru.md).
Двенадцать строковых Action используют общие версионные сервисы JIT;
[контракты и проверки](doc/JIT_STRINGS.ru.md).
InitArray/InitObject сохраняют версионные правила счётчика и элементов;
[контракты и проверки](doc/JIT_LITERALS.ru.md).
Equals/Less и And/Or используют общие версионные преобразования и hook;
[контракты и проверки](doc/JIT_LEGACY.ru.md).
Extends/Implements/InstanceOf/Cast используют общие сервисы с корнями GC;
[контракты и проверки](doc/JIT_CLASSES.ru.md).
GetProperty/SetProperty/TargetPath используют общие нативные сервисы сцены;
[контракты и проверки](doc/JIT_DISPLAY.ru.md).
Нативные Trace/Throw/GetTime/RandomNumber: [контракты и проверки](doc/JIT_EFFECTS.ru.md).
Enumerate/Enumerate2: [контракты и проверки](doc/JIT_ENUMERATE.ru.md).
Нативные команды таймлайна: [контракты и проверки](doc/JIT_TIMELINE.ru.md).
CloneSprite/RemoveSprite/StartDrag/EndDrag: [контракты и проверки](doc/JIT_SCENE.ru.md).
WaitForFrame/WaitForFrame2: [контракты и проверки](doc/JIT_WAIT_FRAMES.ru.md).
GetURL/GetURL2/Call и JIT кадров: [контракты и проверки](doc/JIT_DISPATCH.ru.md).

Контрольный коммит зависимостей: `extern/` объединена с `external/`.
Включены исходники и настройки Cairo 1.16.0, Pixman 0.38.4 и SQLite 3.50.4.
Конкурирующие варианты выбираются в корневом [DEFS.txt](DEFS.txt); по умолчанию
AGG + SQLite + HEADLESS. См. [ключи сборки](doc/BUILD_DEFINITIONS.ru.md) и
[проверки](doc/DEPENDENCY_CHECKPOINT.ru.md). Экранные ключи пока зарезервированы.

Этап стандартной числовой библиотеки: добавлены Math и преобразования
примитивов. См. [числовой API](doc/STANDARD_NUMERIC.ru.md) для установки,
проверенного поведения и ограничений. Историческая контрольная точка: 95 Linux-тестов и примеров прошли.
Текущая команда пользователя на обновление зеркала указана выше.

Этап разработки SWF 8: загрузка FWS/CWS с лимитами, все типы AVM1 Push,
пулы/4 регистра, C callbacks по образцу Lua, исходники zlib/AGG.
VM пока частичная. Доступен явный экспериментальный JIT для подмножества AVM1
на Linux x86-64; полного проигрывателя пока нет. См. [JIT](doc/JIT.md),
[план совместимости](doc/SWF8_ROADMAP.ru.md), [загрузку](doc/SWF_LOADING.ru.md),
[API расширений](doc/LUA_STYLE_API.ru.md), [сборку](doc/BUILD_PLATFORMS.ru.md).

# FlashVM — документация на русском языке

FlashVM — экспериментальная библиотека C++98 с публичным C89 API для SWF 1–8 и частичной реализации AVM1. **Это не готовый Flash Player.**

## Сборка

Требуются CMake, компиляторы C89 и C++98. Команды: `cmake -S . -B build`, `cmake --build build`, `ctest --test-dir build --output-on-failure`.

## Состав API

- `flashvm/flashvm.h`: конфигурация, Runtime, Context, загрузка SWF из памяти. [`fvm_advance`](include/flashvm/flashvm.h#L74) пока возвращает [`FVM_UNSUPPORTED`](include/flashvm/flashvm.h#L46).
- `flashvm/value.h`: значения, объекты, глобальные переменные, нативные функции; объекты принадлежат контексту, указатели не сохранять после его уничтожения.
- `flashvm/swf.h`: анализ FWS/CWS до SWF 8, без ZWS.
- `flashvm/avm1.h`: анализ и частичное выполнение AVM1; JIT отсутствует.
- `flashvm/input.h`: правила сопоставления жестов с действиями мыши, C-обработчик событий. Подробности на русском и английском: [INPUT_EVENTS.md](doc/INPUT_EVENTS.md).
- `flashvm/sqlite.h`: необязательный адаптер SQLite, включается ключом SQLITE из DEFS.txt; пока не интегрирован с ActionScript.

## Примеры

Каталоги [examples/embedding](examples/embedding) и [examples/extending](examples/extending) содержат примеры C и AS2. В README каждой подпапки указано, какие функции работают, а какие только запланированы. Новый [gesture_dispatch](examples/embedding/gesture_dispatch/README.md) показывает тап двумя пальцами и сопоставление с правой кнопкой мыши.

## Ограничения и безопасность

Нет полной совместимости Flash Player 8, JIT, рендеринга, аудио/видео и полноценного диспетчера событий AVM1. Не запускайте недоверенные SWF как изолированные сценарии: песочница не реализована. Поддержка C89/C++98 относится к исходникам FlashVM; сторонние библиотеки могут требовать более новые инструменты.

## Документация API

Подробные документы на английском находятся в `doc/`. Русский обзор доступен в этом файле; двуязычная спецификация ввода — в `doc/INPUT_EVENTS.md`. Для остальных документов добавлены русские сопроводительные файлы `.ru.md` с описанием возможностей и ограничений; подробные сигнатуры остаются в английских оригиналах.

## Потоки отрисовки

Добавлен переносимый исполнитель заданий отрисовки через переходники
`ports/windows`, `ports/linux`, `ports/mac`, `ports/android`.
[Описание API на русском и английском](doc/RENDER_THREADS.md),
[пример на C89](examples/embedding/parallel_render/README.md).
Пока это механизм задач хоста, а не готовый рендерер MovieClip.

## Платформенные адаптеры ввода и сети

Подробности: [API платформенных адаптеров](doc/OS_PORTS.ru.md) и [English](doc/OS_PORTS.md). Реализации: `ports/{windows,linux,mac,android}`, пример на C89: `examples/embedding/os_ports`. Сеть пока блокирующая и не доступна напрямую из ActionScript; события ввода ещё не вызывают обработчики Flash автоматически.

## Этап 14: распознавание касаний

`flashvm/gesture_recognizer.h` распознаёт тапы, мультитапы и свайпы из
событий приложения-хоста с настраиваемыми порогами. Подробности:
`doc/GESTURE_RECOGNIZER.md`, пример:
`examples/embedding/gesture_recognizer/`. Пока события поступают в C callback,
а не в обработчики объектов AVM1.

### Отложенные события ввода (этап 15)

См. [документацию очереди ввода (RU/EN)](doc/INPUT_QUEUE.md) и
[`examples/embedding/input_queue`](examples/embedding/input_queue).
Очередь предназначена для **одного потока** и пока не вызывает обработчики
ActionScript у MovieClip. Межпоточная передача событий остаётся задачей хоста.

- [Нативные методы AVM1 / Native methods](doc/AVM1_METHODS.md)
- [Пример методов объектов на C89](examples/embedding/object_methods/README.md)

## Этап 17: вызовы скриптовых методов

Добавлена начальная поддержка `CallMethod` для именованных функций AVM1 с
параметрами и привязкой `this` к объекту-получателю. Подробности, ограничения
и пример C89/AS: [документация RU/EN](doc/AVM1_SCRIPT_METHODS.md),
[пример](examples/embedding/script_methods/README.md).
Функции как свойства объектов пока не поддерживаются.

## Инспекция таблиц (постепенная реализация)

См. `doc/TABLES_DEBUG.md` и `examples/embedding/tables_debug/`: API таблиц и начальной инспекции отладчика на C89. Окружения функций в стиле Lua, полный протокол метаметодов и пошаговое выполнение **пока не реализованы**.

### Собственные контейнеры и метатаблицы

Описание реализации C++98, ограничений и пример на C89 находятся в
[документации контейнеров и метатаблиц](doc/CUSTOM_CONTAINERS.md).

### Собственные внутренние контейнеры

В ядре C++98 вместо `std::string`, `std::vector` и `std::map` используются собственные реализации из `src/details/`. Подробности, ограничения, текущее состояние гибридных таблиц и метатаблиц, примеры: [документация на русском](doc/INTERNAL_CONTAINERS.ru.md) и [английская версия](doc/INTERNAL_CONTAINERS.md).

## Переносимые текстовые функции (этап 21)

См. [Текстовый API](doc/TEXT_API.md): проверка, чтение и запись UTF-8,
смещения символов и ограниченный движок регулярных выражений по байтам.
Рабочий пример находится в `examples/embedding/text/`. Исходники движка
регулярных выражений — в `external/`. Профили встроенных графических библиотек и SQLite
описаны в [external/README.md](external/README.md).

### Внешние зависимости и гибридная таблица

Состояние включения исходников библиотек, ограничения автономной сборки и
устройство нового контейнера описаны в
[документации](doc/EXTERNAL_DEPENDENCIES.ru.md).

### Этап 23: нативные метаметоды

См. [описание API](doc/METAMETHODS.ru.md) и `tests/metamethods.c`:
`__index`, `__newindex`, `__iterate` и явная финализация `__gc`.
Автоматического сборщика мусора и синтаксиса AS `pairs()` пока нет.

### Стековые расширения C89/C++98 (этап 24)

Lua-подобный интерфейс описан [по-русски](doc/STACK_API.ru.md) и [по-английски](doc/STACK_API.md). Рабочие примеры находятся в `examples/extending/stack_c/` и `examples/embedding/cpp98_stack/`. Это не ABI Lua; непосредственный вызов стековых функций из AVM1 ещё не реализован.

### Вызываемые объекты C (этап 25)

См. [описание API](doc/CALLABLE_OBJECTS.ru.md) и
[пример C89](examples/extending/callable_objects/README.ru.md).
Нативные C-функции можно сохранять как значения объектов и вызывать через C API.
Замыкания AVM1 и `__call` пока не реализованы.

## Нативный `__call` (этап 26)

`include/flashvm/meta_call.h` предоставляет [`fvm_meta_call`](include/flashvm/meta_call.h#L20): обработчик получает исходный объект, метатаблицу и отдельную индексированную таблицу аргументов. Рабочий тест — `tests/meta_call.c`. Опциональные объекты потоков и каналов ActionScript реализованы через [`fvm_extensions_install`](include/flashvm/extensions.h#L17); проектный контракт описан в `doc/THREADS_CHANNELS.ru.md`.

### Внутренние каналы (этап 27)

См. [описание API каналов](doc/CHANNELS.ru.md) и [пример C89](examples/embedding/channels/README.md). Добавлена переносимая синхронизированная FIFO-очередь для C/C++. Потоки VM и методы каналов ActionScript доступны через [`fvm_extensions_install`](include/flashvm/extensions.h#L17); нестандартные операторы языка пока отсутствуют.

### Нативные рабочие потоки

См. [API потоков](doc/WORKERS.ru.md) и [пример C89](examples/embedding/workers/). Потоки создают независимые контексты интерпретатора через переходники `ports/`. ActionScript `thread.run` доступен после [`fvm_extensions_install`](include/flashvm/extensions.h#L17); см. `doc/ACTIONSCRIPT_EXTENSIONS.ru.md`.

Управление жизненным циклом потоков (отложенный запуск, состояния, ожидание):
[doc/WORKER_LIFECYCLE.ru.md](doc/WORKER_LIFECYCLE.ru.md).

### Кооперативная остановка рабочих потоков

Переносимый C89 API поддерживает запрос остановки потока и ожидание массива
потоков. См. [документацию](doc/WORKER_CANCELLATION.ru.md) и
`examples/embedding/worker_stop/`. Объект ActionScript `thread` доступен через [`fvm_extensions_install`](include/flashvm/extensions.h#L17).

## Обмен сообщениями между потоками

Рабочий пример C89 находится в [`examples/embedding/worker_channels/`](examples/embedding/worker_channels/). В [`doc/WORKER_CHANNELS.ru.md`](doc/WORKER_CHANNELS.ru.md) описаны блокирующее и неблокирующее чтение, время жизни строк, порядок остановки и ограничения.

Ограниченные очереди каналов и блокировка при заполнении: см. [doc/CHANNEL_CAPACITY.ru.md](doc/CHANNEL_CAPACITY.ru.md).

Совместное владение каналами описано в [doc/CHANNEL_OWNERSHIP.md](doc/CHANNEL_OWNERSHIP.md).

### Каналы с управляемым владением (этапы 34–38)

См. [русское описание API](doc/CHANNEL_VALUES.ru.md),
[английское описание](doc/CHANNEL_VALUES.md), примеры C89/C++98 в
`examples/embedding/channel_values`, `channel_cpp98` и
`examples/extending/channel_worker`. Пока это нативные дескрипторы, **не**
управляемые GC значения ActionScript.

## Сборщик мусора (экспериментальный)

`flashvm/gc.h` предоставляет сборщик mark-and-sweep для каждого контекста,
удержание объектов из C, настройку частоты и однократную финализацию.
Недостижимые **циклические ссылки** освобождаются. Корни стека AVM1 ещё
не интегрированы, поэтому сборку следует запускать только в явно безопасных
точках. См. [`doc/GARBAGE_COLLECTION.ru.md`](doc/GARBAGE_COLLECTION.ru.md)
и [`examples/embedding/gc/`](examples/embedding/gc/).
Объект ActionScript `gc` с методами `mode` и `state` пока только проектируется.

## Документация GC (этапы 49–68)

См. [безопасные точки](doc/GC_SAFE_POINTS.md), [режимы GC](doc/GC_MODES.ru.md), [указатель API](doc/API_INDEX.ru.md) и [диагностику](doc/GC_TROUBLESHOOTING.md). Сборку пока нельзя безопасно запускать внутри активных кадров AVM1.

Корни GC активных кадров: [English](doc/GC_ACTIVE_FRAMES.md) / [Русский](doc/GC_ACTIVE_FRAMES.ru.md).

### Временные корни GC

См. [временные области корней](doc/GC_NATIVE_ROOTS.ru.md): встраивание из C89, правила владения и ограничения.

### Безопасность GC и диагностика

См. [Безопасность GC при нативных вызовах](doc/GC_CALLBACK_SAFETY.ru.md):
защита от повторного запуска, корни и диагностика через C89 API.

### Автоматическая сборка мусора в AVM1

Интерпретатор проверяет GC между инструкциями при зарегистрированных корнях
активных кадров. Подробнее: [безопасные точки GC](doc/GC_AVM1_SAFE_POINTS.ru.md).

## Проверенное продолжение разработки

[Расширения ActionScript](doc/ACTIONSCRIPT_EXTENSIONS.ru.md), [контракты GC](doc/GC_COMPLETION.ru.md), [владение потоками и каналами](doc/CONCURRENCY_SAFETY.ru.md), [результаты проверок](doc/VALIDATION.ru.md). Добавлены ограниченные проходы финализации, release userdata, корни захваченных нативными callback объектов, безопасный одновременный join, независимое владение строками сообщений, снимки циклических графов и опциональные объекты `gc`/`thread`/`channel`.

Сборка по умолчанию не требует внешних системных библиотек. SQLite встроен исходниками и включён по умолчанию через DEFS.txt; исходники встроенного regex находятся в `external/`. На Linux без CMake: `python3 scripts/check.py`; ASan/UBSan: добавить `--sanitize`. Ограничения LeakSanitizer и платформ указаны в отчёте.

Файловые каналы и метатаблицы значений: [по-русски](doc/FILE_CHANNELS.ru.md), [по-английски](doc/FILE_CHANNELS.md).
Текущий ABI — 2: нативные клиенты необходимо пересобрать. [Отчёт и ограничения](doc/FILE_CHANNELS_CHECKPOINT.ru.md).

AVM1 decomposition / Декомпозиция: value-based C services and prepared native script
bodies for future JIT reuse; file map, GC contract, tests and limitations:
[AVM1_DECOMPOSITION.md](doc/AVM1_DECOMPOSITION.md).

Raw prototype primitive retention / Raw-значения прототипа: [RU/EN](doc/OBJECT_RAW_PROTOTYPE.md).

Shared execution budget/GC/stop checkpoints / Общие контрольные точки: [RU/EN](doc/EXECUTION_CHECKPOINT.md).

Loaded numeric text and strict NaN / Числовой текст и строгий NaN: [RU/EN](doc/AVM1_NUMBER_TEXT.md).

Named uppercase constants / Смысловые макросы: [header map and evidence](doc/SEMANTIC_CONSTANTS.md).

В v0.348 добавлен явный экспериментальный JIT Linux x86-64.
[Сборка и запуск примера интерпретатор/JIT](doc/JIT.md); полный JIT остаётся открытым.

Режимы интерпретации/JIT при вызове, переключение во время исполнения и
диагностический повтор после исключения: [API и пример](doc/EXECUTION_MODES.md).

Multi-architecture JIT, breakpoints, threads and multiple VM examples / JIT для разных CPU, отладка, потоки и несколько VM: [JIT_ARCHITECTURES.md](doc/JIT_ARCHITECTURES.md).

Подробное руководство: [JIT, отладка, потоки и несколько VM](doc/JIT_ARCHITECTURES.ru.md).

Нативные DefineFunction/DefineFunction2: [English](doc/JIT_DEFINITIONS.md) /
[Русский](doc/JIT_DEFINITIONS.ru.md); захват области/пула, callbacks и отказы памяти.

Нативный With: общий стек/регистры/пул, вложенный Return и восстановление области;
[English](doc/JIT_WITH.md) / [Русский](doc/JIT_WITH.ru.md).

Нативные Try/Catch/Finally: сохранение исключений/Return, catch по имени/регистру
и защита root-входа от OOM; [English](doc/JIT_TRY.md) / [Русский](doc/JIT_TRY.ru.md).

Borrowed negative splice / отрицательная длина: [EN](doc/ARRAY_NEGATIVE_SPLICE.md) / [RU](doc/ARRAY_NEGATIVE_SPLICE.ru.md).

Signed borrowed end operations / знаковые заимствованные методы: [EN](doc/ARRAY_NEGATIVE_ENDS.md) / [RU](doc/ARRAY_NEGATIVE_ENDS.ru.md).

Negative loaded slice / slice с отрицательной длиной: [EN](doc/ARRAY_NEGATIVE_SLICE.md) / [RU](doc/ARRAY_NEGATIVE_SLICE.ru.md).

Large loaded Array mutations / большие загруженные массивы: [EN](doc/ARRAY_LOADED_LARGE_MUTATION.md) / [RU](doc/ARRAY_LOADED_LARGE_MUTATION.ru.md).

Large loaded sorting / сортировка больших массивов: [EN](doc/ARRAY_LOADED_LARGE_SORT.md) / [RU](doc/ARRAY_LOADED_LARGE_SORT.ru.md).

Unicode legacy display paths / Unicode-пути старых SWF: [EN](doc/MOVIECLIP_UNICODE_PATHS.md) / [RU](doc/MOVIECLIP_UNICODE_PATHS.ru.md).

[Интерфейсы оригинальных функций](doc/REMOTE_FUNCTIONS.ru.md) — v0.474.

[Примеры RPC функций: процессы, независимые VM-потоки и LAN](doc/FUNCTION_RPC_EXAMPLES.ru.md).

[Автоматическое обновление версии в CMake](doc/BUILD_VERSION.ru.md).

[Загруженный Object.__resolve](doc/OBJECT_RESOLVE.ru.md) — v0.474: обработчик
отсутствующего свойства, counted-имена, регистр SWF, корни GC и предел рекурсии.

[Пример resolver на C89](examples/embedding/property_resolve/README.ru.md).

[Loaded builtin construction / Конструирование и вызовы SWF](doc/BUILTIN_CONSTRUCTION.md) — v0.474.

[Current-global primitive boxing / Текущие классы примитивов](doc/PRIMITIVE_GLOBAL_BOXING.md) — v0.474.

Debugger tutorials / Примеры отладчика: [examples/debugger](examples/debugger/README.md).
Complete API reference: [English](doc/DEBUG_API.md) / [Русский](doc/DEBUG_API.ru.md);
[SWF/SWD source mapping](doc/SWD_DEBUGGING.md).

Полный каталог примеров / Complete example catalog: [examples/README.md](examples/README.md).
Documentation source links and maintenance / Правила документации: [RU/EN](doc/DOCUMENTATION_GUIDELINES.md).
Stage quality / Качество сцены: [API and ToggleQuality](doc/STAGE_QUALITY.md).
[Multimedia and device bridges / Мосты мультимедиа и устройств](doc/MULTIMEDIA_BRIDGES.md): static FFmpeg and replaceable host callbacks.
[Native speech / Системные TTS и STT](doc/SPEECH_BRIDGES.md): explicit providers and per-OS prepared-host examples.
## Лицензия

Собственный код и документация FlashVM распространяются по
[лицензии MIT](../LICENSE). Copyright (c) 2026 bhgv.
Сторонний код, данные и зависимости сохраняют свои авторские уведомления и
лицензии: корневая MIT их не заменяет. См. уведомления в
[external/](external/README.md), каталогах соответствующих зависимостей и
отмеченных сторонних фрагментах исходников проекта.
## Локальный просмотр документации

Установите `python -m pip install -r doku_viewer/requirements.txt`, затем
запустите `python view_docs.py`. Python 2.7.9/3, навигация по Markdown и
подсвеченные исходники с переходом к строке: [руководство](doku_viewer/README.md).
## Десктопное приложение

[Frontend](frontend/README.ru.md): необязательный SDL2-проигрыватель для
Linux/Windows/macOS, SWF-пути, JIT/interpreter, качество, FPS/проценты и справка.
[Полное руководство](doc/DESKTOP_FRONTEND.ru.md).
Порционный StartSound-микшер: [руководство](doc/SWF_SOUND_MIXER.ru.md),
[учебник](examples/multimedia/swf_event_mixer/README.md); перекрытие голосов,
stop/no-multiple, циклы и огибающие без новых выделений при чтении.
Root timeline sound delivery: [EN](doc/MOVIECLIP_SOUND.md) /
[RU](doc/MOVIECLIP_SOUND.ru.md); actual frame entry before DoAction,
source checks and host-local callback registration.
Explicit mixed-rate/channel audio: [EN](doc/SWF_SOUND_CONVERSION.md) /
[RU](doc/SWF_SOUND_CONVERSION.ru.md); configured output including 48000 Hz stereo,
with legacy strict API preserved and an annotated --convert-demo tutorial.
v0.583: отдельный audio worker и [StopSounds](doc/AUDIO_STOP_BINDINGS.ru.md); исправление треска Android требует проверки на телефоне.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_advance` | function | [include/flashvm/flashvm.h:74](include/flashvm/flashvm.h#L74) |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:17](include/flashvm/extensions.h#L17) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](include/flashvm/meta_call.h#L20) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->

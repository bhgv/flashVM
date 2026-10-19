# Каталог примеров FlashVM / FlashVM example catalog

## Оглавление

- [actionscript](#group-actionscript)
  - [language](#group-actionscript-language)
    - [error.as](#example-actionscript-error-as) — <small><em>Error, catch и гарантированный finally</em></small>
    - [parsing.as](#example-actionscript-parsing-as) — <small><em>Числа из строк и выбор основания</em></small>
    - [string.as](#example-actionscript-string-as) — <small><em>UTF-16, поиск и подстроки с emoji</em></small>
  - [resources](#group-actionscript-resources)
    - [file_channels.as](#example-actionscript-file_channels-as) — <small><em>Сохранение графа в файл и чтение обратно</em></small>
    - [gc_workers_channels.as](#example-actionscript-gc_workers_channels-as) — <small><em>Worker, канал и очистка через GC</em></small>
    - [resource_exceptions.as](#example-actionscript-resource_exceptions-as) — <small><em>Ошибки ресурсов и освобождение mutex</em></small>
    - [shared_mutex.as](#example-actionscript-shared_mutex-as) — <small><em>Общий mutex и канал между worker</em></small>
    - [sprite_resources.as](#example-actionscript-sprite_resources-as) — <small><em>Экземпляры спрайта из ресурсов SWF</em></small>
- [debugger](#group-debugger)
  - [inspection](#group-debugger-inspection)
    - [channel_inspection](#example-debugger-channel_inspection) — <small><em>Заморозка очереди и просмотр сообщения</em></small>
    - [resource_snapshots](#example-debugger-resource_snapshots) — <small><em>Объекты, функции и владельцы mutex</em></small>
  - [stops](#group-debugger-stops)
    - [backtrace](#example-debugger-backtrace) — <small><em>JSON-бэктрейс после ошибки VM</em></small>
    - [jit_dump](#example-debugger-jit_dump) — <small><em>ZIP-дамп VM с исходным и JIT-кодом</em></small>
    - [live_variables](#example-debugger-live_variables) — <small><em>Живые аргументы, локальные и регистры</em></small>
    - [source_breakpoints](#example-debugger-source_breakpoints) — <small><em>SWF/SWD, breakpoint и строка исходника</em></small>
- [embedding](#group-embedding)
  - [channels](#group-embedding-channels)
    - [channel_cpp98](#example-embedding-channel_cpp98) — <small><em>RAII-владение каналом в C++98</em></small>
    - [channel_values](#example-embedding-channel_values) — <small><em>Копирование и освобождение channel handle</em></small>
    - [channels](#example-embedding-channels) — <small><em>Отправка и получение через FIFO</em></small>
    - [file_channels](#example-embedding-file_channels) — <small><em>Перенос графа через файловый канал</em></small>
    - [resource_broker](#example-embedding-resource_broker) — <small><em>Живой ресурс через TCP-брокер</em></small>
    - [shared_mutex](#example-embedding-shared_mutex) — <small><em>Общая блокировка из разных VM</em></small>
  - [functions](#group-embedding-functions)
    - [cpp98_stack](#example-embedding-cpp98_stack) — <small><em>RAII-стек значений и индексы C++98</em></small>
    - [function_parameters](#example-embedding-function_parameters) — <small><em>Параметры AVM1-функции — AS-исходник</em></small>
    - [function_rpc](#example-embedding-function_rpc) — <small><em>Вызовы между потоками, процессами и LAN</em></small>
    - [object_methods](#example-embedding-object_methods) — <small><em>Native-метод, receiver и аргументы</em></small>
    - [script_functions](#example-embedding-script_functions) — <small><em>Объявление script-функции — AS-исходник</em></small>
    - [script_methods](#example-embedding-script_methods) — <small><em>Метод объекта и this — AS-исходник</em></small>
  - [gc](#group-embedding-gc)
    - [cycle_collection](#example-embedding-cycle_collection) — <small><em>Недостижимые циклы — заметка о GC</em></small>
    - [gc](#example-embedding-gc) — <small><em>Pin/unpin объекта и ручная сборка</em></small>
    - [manual_collection](#example-embedding-manual_collection) — <small><em>Безопасные точки ручной GC — заметка</em></small>
    - [mode_tuning](#example-embedding-mode_tuning) — <small><em>Режим и порог сборщика — заметка</em></small>
    - [resurrection](#example-embedding-resurrection) — <small><em>Возврат объекта из финализатора — заметка</em></small>
  - [graphics](#group-embedding-graphics)
    - [bitmap_fill](#example-embedding-bitmap_fill) — <small><em>Живой BitmapData как заливка клипа</em></small>
    - [color_matrix_filter](#example-embedding-color_matrix_filter) — <small><em>Изменение RGBA через матрицу цвета</em></small>
    - [drawing](#example-embedding-drawing) — <small><em>Рисование и host-интеграция — AS-эскиз</em></small>
    - [geometry](#example-embedding-geometry) — <small><em>Point, Matrix и Rectangle — AS-исходник</em></small>
    - [gradients](#example-embedding-gradients) — <small><em>Векторный путь с градиентной заливкой</em></small>
    - [image_codecs](#example-embedding-image_codecs) — <small><em>Разбор SVG, растеризация и PNG</em></small>
    - [media](#example-embedding-media) — <small><em>Подключение звука и видео — AS-эскиз</em></small>
    - [movieclip](#example-embedding-movieclip) — <small><em>Управление MovieClip — AS-эскиз</em></small>
    - [parallel_render](#example-embedding-parallel_render) — <small><em>Независимые задания рендеринга в потоках</em></small>
    - [stage_quality](#example-embedding-stage_quality) — <small><em>ToggleQuality и края треугольника LOW/HIGH</em></small>
  - [input](#group-embedding-input)
    - [gesture_dispatch](#example-embedding-gesture_dispatch) — <small><em>Жест преобразуется в C callback мыши</em></small>
    - [gesture_recognizer](#example-embedding-gesture_recognizer) — <small><em>Распознавание tap по событиям касания</em></small>
    - [input](#example-embedding-input) — <small><em>Правила преобразования tap и swipe</em></small>
    - [input_queue](#example-embedding-input_queue) — <small><em>Очередь ввода и обработка на VM-потоке</em></small>
    - [keyboard](#group-embedding-input-keyboard)
      - [keyboard_listeners](#example-embedding-keyboard_listeners) — <small><em>Слушатели Key, явный выбор клипа и короткие нажатия</em></small>
      - [keyboard_polling](#example-embedding-keyboard_polling) — <small><em>Опрос Key, одновременные клавиши и сброс фокуса</em></small>
      - [selection](#example-embedding-selection) — <small><em>Selection: выбор клипа, уведомления и наблюдатели</em></small>
    - [mixed_input](#example-embedding-mixed_input) — <small><em>Одна очередь Mouse/Key, снимки и отмена пакета</em></small>
    - [mouse](#group-embedding-input-mouse)
      - [mouse_listeners](#example-embedding-mouse_listeners) — <small><em>Слушатели указателя: касание, движение и отпускание</em></small>
  - [jit](#group-embedding-jit)
    - [execution_modes](#example-embedding-execution_modes) — <small><em>Режимы исполнения и replay ошибки</em></small>
    - [jit](#example-embedding-jit) — <small><em>Ручной JIT, сравнение с интерпретатором и подготовка ОС</em></small>
    - [jit_debug](#example-embedding-jit_debug) — <small><em>Source map, breakpoint и статистика JIT</em></small>
    - [jit_instances](#example-embedding-jit_instances) — <small><em>Две VM, строгий JIT и вызов после teardown другой VM</em></small>
    - [jit_workers](#example-embedding-jit_workers) — <small><em>Два worker с отдельными VM и JIT</em></small>
  - [objects](#group-embedding-objects)
    - [main](#example-embedding-main) — <small><em>Runtime, объект и nativeSum: базовый C-хост</em></small>
    - [objects](#example-embedding-objects) — <small><em>Объекты, свойства и глобальные значения</em></small>
    - [property_resolve](#example-embedding-property_resolve) — <small><em>Ленивое разрешение свойств через __resolve</em></small>
    - [prototypes](#example-embedding-prototypes) — <small><em>Цепочка прототипов и унаследованные поля</em></small>
    - [standard_boolean](#example-embedding-standard_boolean) — <small><em>Boolean как функция и boxed-объект</em></small>
    - [tables_debug](#example-embedding-tables_debug) — <small><em>Именованные поля, индексы и инспекция</em></small>
  - [platform](#group-embedding-platform)
    - [network](#example-embedding-network) — <small><em>Сетевой host bridge — AS-эскиз</em></small>
    - [os_ports](#example-embedding-os_ports) — <small><em>Native-ввод и TCP connect/send/close</em></small>
  - [text](#group-embedding-text)
    - [font_libraries](#example-embedding-font_libraries) — <small><em>Шрифты, shaping и редактируемые контуры</em></small>
    - [standard_text](#example-embedding-standard_text) — <small><em>TextField, шрифты и векторный текст</em></small>
    - [text](#example-embedding-text) — <small><em>Проверка UTF-8 и поиск регулярным выражением</em></small>
    - [text_layout](#example-embedding-text_layout) — <small><em>Строки, метрики и контуры text layout</em></small>
  - [workers](#group-embedding-workers)
    - [worker_channels](#example-embedding-worker_channels) — <small><em>Обмен host↔worker по двум каналам</em></small>
    - [worker_lifecycle](#example-embedding-worker_lifecycle) — <small><em>Отложенный запуск, state и join</em></small>
    - [worker_stop](#example-embedding-worker_stop) — <small><em>Кооперативная остановка группы worker</em></small>
    - [workers](#example-embedding-workers) — <small><em>Native callback в потоке со своей VM</em></small>
- [extending](#group-extending)
  - [callbacks](#group-extending-callbacks)
    - [callable_objects](#example-extending-callable_objects) — <small><em>Вызываемый объект со стеком аргументов</em></small>
    - [lua_style](#example-extending-lua_style) — <small><em>Native add через Lua-подобный API</em></small>
    - [main](#example-extending-main) — <small><em>Регистрация makePoint и возврат объекта</em></small>
    - [meta_call](#example-extending-meta_call) — <small><em>Явный __call — описание тестового сценария</em></small>
    - [native_functions](#example-extending-native_functions) — <small><em>Вызов функции хоста — AS-эскиз</em></small>
    - [stack_c](#example-extending-stack_c) — <small><em>Стековый C callback и один результат</em></small>
  - [integration](#group-extending-integration)
    - [channel_worker](#example-extending-channel_worker) — <small><em>Worker с удерживаемым channel handle</em></small>
    - [drawing](#example-extending-drawing) — <small><em>Drawing bridge расширения — AS-эскиз</em></small>
    - [media](#example-extending-media) — <small><em>Медиа bridge расширения — AS-эскиз</em></small>
    - [movieclip](#example-extending-movieclip) — <small><em>MovieClip в расширении — AS-эскиз</em></small>
    - [network](#example-extending-network) — <small><em>Сетевое расширение — AS-эскиз</em></small>
    - [prototypes](#example-extending-prototypes) — <small><em>Native-объекты на прототипах — заметка</em></small>
    - [sqlite](#example-extending-sqlite) — <small><em>SQLite в памяти и целочисленный запрос</em></small>
- [frontends](#group-frontends)
  - [memory](#group-frontends-memory)
    - [headless](#example-frontends-headless) — <small><em>Кадр и ввод без оконного SDK</em></small>
  - [windows](#group-frontends-windows)
    - [clutter3d](#example-frontends-clutter3d) — <small><em>Текстура и касания в Clutter/Cogl из выбранного Git</em></small>
    - [sdl](#example-frontends-sdl) — <small><em>SDL2: streaming RGBA, мышь и первое касание</em></small>
- [multimedia](#group-multimedia)
  - [bridges](#group-multimedia-bridges)
    - [alsa_output](#example-multimedia-alsa_output) — <small><em>Linux ALSA: PCM-устройство и StopSounds</em></small>
    - [audio_channels](#example-multimedia-audio_channels) — <small><em>Регистрация PCM-выхода и остановка через AVM1</em></small>
    - [custom_audio_bridge](#example-multimedia-custom_audio_bridge) — <small><em>Свой источник PCM через stdio</em></small>
    - [custom_video_output](#example-multimedia-custom_video_output) — <small><em>Свой вывод RGBA в реальный PPM файл</em></small>
    - [stop_binding](#example-multimedia-stop_binding) — <small><em>Остановка подготовленного звука через stopAllSounds</em></small>
  - [decoding](#group-multimedia-decoding)
    - [ffmpeg_decode](#example-multimedia-ffmpeg_decode) — <small><em>FFmpeg: декодирование файлов и явная камера</em></small>
    - [swf_audio](#example-multimedia-swf_audio) — <small><em>DefineSound/StartSound: ресурсы, события и экспорт PCM</em></small>
    - [swf_stream](#example-multimedia-swf_stream) — <small><em>Потоковый SWF PCM: блоки, независимый индекс и native FILE</em></small>
    - [swf_stream_mp3](#example-multimedia-swf_stream_mp3) — <small><em>MP3-префиксы и сжатый снимок после уничтожения VM</em></small>
  - [devices](#group-multimedia-devices)
    - [system_devices](#example-multimedia-system_devices) — <small><em>Свой опрос устройств через Linux evdev</em></small>
  - [mixing](#group-multimedia-mixing)
    - [swf_audio_queue](#example-multimedia-swf_audio_queue) — <small><em>Сменная FIFO PCM, повтор записи, часы потребления и SDL2</em></small>
    - [swf_event_mixer](#example-multimedia-swf_event_mixer) — <small><em>StartSound-микшер: manual/timeline, 48 kHz stereo и stop</em></small>
    - [swf_mix](#example-multimedia-swf_mix) — <small><em>Два голоса в PCM-файл; StopSounds через interpreter/JIT</em></small>
    - [swf_polyphony](#example-multimedia-swf_polyphony) — <small><em>Несколько голосов, no-multiple и stop по ресурсу</em></small>
  - [speech](#group-multimedia-speech)
    - [speech_android](#example-multimedia-speech_android) — <small><em>Android: Context, permissions, JNI и речевые службы</em></small>
    - [speech_macos](#example-multimedia-speech_macos) — <small><em>macOS: реальные speech и microphone permissions</em></small>
    - [speech_stt](#example-multimedia-speech_stt) — <small><em>Системный STT: разрешения и захват настоящей речи</em></small>
    - [speech_tts](#example-multimedia-speech_tts) — <small><em>Системный TTS: подготовка голоса и реальная речь</em></small>
- [player](#group-player)
  - [animation_audio](#example-player-animation_audio) — <small><em>Три SWF: анимация, клавиши/мышь и PCM/ADPCM в interpreter/JIT</em></small>
- [Сборка, типы примеров и чтение API](#using-examples)
- [Глоссарий — алфавитный список всех примеров](#glossary)

<a id="using-examples"></a>
## Сборка, типы примеров и чтение API

Основные группы соответствуют подпапкам `actionscript/`, `debugger/`,
`embedding/`, `extending/`, `frontends/`, `multimedia/`, `player/`. Тематические подгруппы ниже объединяют одну идею;
имя подгруппы занимает своё алфавитное место, а её примеры также отсортированы.
Тематические группы обычно сохраняют пути и CMake-имена; перенос frontend-хоста явно отмечен выше. Например, например, подгруппа
`embedding/jit` в каталоге объединяет `jit/`, `jit_debug/`, `jit_instances/` и
`jit_workers/`. Вспомогательные headers и сопровождающие AS-файлы описываются
в составе своего примера, а не выдаются за самостоятельные программы.

У каждого пункта указаны тип, исходники, назначение и ссылка на локальный README.
**C/C++** означает наличие main, но доступность возможностей зависит от DEFS.
**ActionScript** требует внешнего компилятора в SWF/AVM1; текст .as не передаётся
FlashVM как программа. **Заметка** не имеет самостоятельного исполняемого файла.
Исторические эскизы не являются подтверждением полноты соответствующей подсистемы.
Точные контракты смотрите по ссылкам API и текущему [роудмапу](../doc/ROADMAP.md).

```sh
cmake -S . -B build-examples -DFVM_DEFS_FILE=cmake/profiles/minimal.defs \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-examples --parallel
ctest --test-dir build-examples -R flashvm_examples_ --output-on-failure
```

Для графики выберите профиль с AGG/Cairo; minimal отключает графические backend.
OS_PORTS, JIT, codecs и SQLite также зависят от профиля. Некоторые программы
пишут файлы или открывают соединения: перед запуском читайте их README.
CMake-цель получается из пути: `embedding/jit/main.c` →
`flashvm_examples_embedding_jit_main`; соответствующее имя также используется
CTest. Программы запускайте из выбранного рабочего каталога, чтобы явно
контролировать размещение создаваемых файлов.

В README каждого примера есть список непосредственно используемых C API
со ссылками на публичные декларации и файлы руководства. Для AS/заметок
отдельно указаны демонстрируемые script API или связанные C API: эти списки
не означают, что в папке присутствует выполняющий их main.

English: this catalog covers every example folder and standalone ActionScript
sample. Groups match the existing directories; conceptual subgroups and entries
are alphabetically sorted. Each entry distinguishes executable C/C++, external-
compiler ActionScript and documentation-only notes. Per-example README files
link demonstrated API functions to public headers and feature guides.

<a id="group-actionscript"></a>
## actionscript

<a id="group-actionscript-language"></a>
### language

<a id="example-actionscript-error-as"></a>
#### error.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/error.md](actionscript/error.md).

Показывает создание Error, throw, проверку instanceof в catch и выполнение finally. Полезен для понимания обработки ошибки на уровне AS1/AS2; исполняемая проверка соответствующего байткода находится в tests/actionscript_error.cpp.

**Файлы:** [error.as](actionscript/error.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-parsing-as"></a>
#### parsing.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/parsing.md](actionscript/parsing.md).

Сравнивает parseInt с автоматическим и явным основанием и parseFloat с числовым префиксом. Примеры 0777, шестнадцатеричного 3E8 и строки с единицами показывают, почему основание и границы числового префикса важны.

**Файлы:** [parsing.as](actionscript/parsing.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-string-as"></a>
#### string.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/string.md](actionscript/string.md).

Показывает String.length, substring, indexOf, charCodeAt и fromCharCode на смешанной латинице, кириллице и emoji. Позволяет отличить число кодовых единиц UTF-16 от числа видимых символов; это исходник для внешнего AVM1-компилятора.

**Файлы:** [string.as](actionscript/string.as).

[К оглавлению](#оглавление).

<a id="group-actionscript-resources"></a>
### resources

<a id="example-actionscript-file_channels-as"></a>
#### file_channels.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/file_channels.md](actionscript/file_channels.md).

Сохраняет циклический граф объектов в файловый канал, закрывает его и читает граф обратно. Различает receive и tryReceive на EOF и описывает режим добавления; расширения устанавливает приложение через [fvm_extensions_install](../include/flashvm/extensions.h#L17).

**Файлы:** [file_channels.as](actionscript/file_channels.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-gc_workers_channels-as"></a>
#### gc_workers_channels.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/gc_workers_channels.md](actionscript/gc_workers_channels.md).

Связывает именованную worker-функцию, отложенный запуск, передачу сообщения по общему каналу и join. После завершения убирает ссылку на worker и вызывает GC; демонстрирует координацию ресурсов между изолированными VM.

**Файлы:** [gc_workers_channels.as](actionscript/gc_workers_channels.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-resource_exceptions-as"></a>
#### resource_exceptions.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/resource_exceptions.md](actionscript/resource_exceptions.md).

Использует try/catch/finally для захвата и гарантированного освобождения mutex. В catch рассматривает структурированные поля ресурсной ошибки, включая транспорт и путь; подходит как шаблон очистки после отказа операции.

**Файлы:** [resource_exceptions.as](actionscript/resource_exceptions.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-shared_mutex-as"></a>
#### shared_mutex.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/shared_mutex.md](actionscript/shared_mutex.md).

Передаёт worker-потоку объект с mutex и каналом. Рабочая функция захватывает общий ресурс, отправляет сообщение и освобождает его; показывает различие общей идентичности ресурса и копирования обычного графа.

**Файлы:** [shared_mutex.as](actionscript/shared_mutex.as).

[К оглавлению](#оглавление).

<a id="example-actionscript-sprite_resources-as"></a>
#### sprite_resources.as

**Тип:** ActionScript — исходник для внешнего AVM1-компилятора. **README:** [actionscript/sprite_resources.md](actionscript/sprite_resources.md).

Создаёт два независимых экземпляра экспортированного спрайта Hero и обращается к его таймлайну. Требует заранее загруженного SWF с ExportAssets и меткой second; показывает использование ресурсов документа, а не создание спрайта из пустого объекта.

**Файлы:** [sprite_resources.as](actionscript/sprite_resources.as).

[К оглавлению](#оглавление).

<a id="group-debugger"></a>
## debugger

<a id="group-debugger-inspection"></a>
### inspection

<a id="example-debugger-channel_inspection"></a>
#### channel_inspection

**Тип:** C/C++ — собираемый пример. **README:** [debugger/channel_inspection/README.md](debugger/channel_inspection/README.md).

Останавливает доставку FIFO, читает состояние очереди и независимый снимок сообщения без его извлечения. Затем возобновляет доставку и принимает исходное значение; все попытки получения неблокирующие.

**Файлы:** [main.c](debugger/channel_inspection/main.c).

[К оглавлению](#оглавление).

<a id="example-debugger-resource_snapshots"></a>
#### resource_snapshots

**Тип:** C/C++ — собираемый пример. **README:** [debugger/resource_snapshots/README.md](debugger/resource_snapshots/README.md).

Перечисляет объекты и реестры функций, затем снимает согласованный снимок владельцев mutex. Показывает двухшаговый запрос размера и копирование данных, не создавая искусственный deadlock.

**Файлы:** [main.c](debugger/resource_snapshots/main.c).

[К оглавлению](#оглавление).

<a id="group-debugger-stops"></a>
### stops

<a id="example-debugger-backtrace"></a>
#### backtrace

**Тип:** C/C++ — собираемый пример. **README:** [debugger/backtrace/README.md](debugger/backtrace/README.md).

Сохраняет JSON-бэктрейс после намеренного Throw и читает его после возврата из функции. Режим BACKTRACE сочетается с NATURAL; пример подходит для логирования без интерфейса отладчика.

**Файлы:** [main.c](debugger/backtrace/main.c).

[К оглавлению](#оглавление).

<a id="example-debugger-jit_dump"></a>
#### jit_dump

**Тип:** C/C++ — собираемый пример. **README:** [debugger/jit_dump/README.md](debugger/jit_dump/README.md).

Явно компилирует AVM1, связывает машинное тело с исходным байткодом и создаёт ZIP_CODE-дамп. Проверяет результат записи отдельно от исходного исключения; при отсутствии JIT демонстрирует интерпретаторный вариант.

**Файлы:** [main.c](debugger/jit_dump/main.c).

[К оглавлению](#оглавление).

<a id="example-debugger-live_variables"></a>
#### live_variables

**Тип:** C/C++ — собираемый пример. **README:** [debugger/live_variables/README.md](debugger/live_variables/README.md).

Входит в синхронный debugger callback до раскрутки кадров и читает аргумент, локальные значения и регистр. FORCE и выбор JIT независимы; callback проверяет успешность сохранения и возвращает управление обычной обработке ошибки.

**Файлы:** [main.c](debugger/live_variables/main.c).

[К оглавлению](#оглавление).

<a id="example-debugger-source_breakpoints"></a>
#### source_breakpoints

**Тип:** C/C++ — собираемый пример. **README:** [debugger/source_breakpoints/README.md](debugger/source_breakpoints/README.md).

Подключает согласованную пару SWF/SWD, ставит breakpoint по смещению и показывает Main.as с текущей строкой. Выбранное тело исполняется интерпретатором; crash action NONE не мешает осмотру исходника.

**Файлы:** [main.c](debugger/source_breakpoints/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding"></a>
## embedding

<a id="group-embedding-channels"></a>
### channels

<a id="example-embedding-channel_cpp98"></a>
#### channel_cpp98

**Тип:** C/C++ — собираемый пример. **README:** [embedding/channel_cpp98/README.md](embedding/channel_cpp98/README.md).

Оборачивает владеющий дескриптор канала в C++98 RAII-класс. Запрещает неявное копирование и освобождает ссылку при выходе из области видимости; полезен для безопасного управления каналом в C++-хосте. Явная копия удерживает дополнительную ссылку на ту же очередь; сообщения не дублируются.

**Файлы:** [main.cpp](embedding/channel_cpp98/main.cpp).

[К оглавлению](#оглавление).

<a id="example-embedding-channel_values"></a>
#### channel_values

**Тип:** C/C++ — собираемый пример. **README:** [embedding/channel_values/README.md](embedding/channel_values/README.md).

Создаёт владеющий channel handle, явно копирует ссылку на тот же канал и передаёт число между двумя handles. Объясняет, почему копия дескриптора не является копией очереди и требует отдельного release.

**Файлы:** [main.c](embedding/channel_values/main.c), [usage.as](embedding/channel_values/usage.as).

[К оглавлению](#оглавление).

<a id="example-embedding-channels"></a>
#### channels

**Тип:** C/C++ — собираемый пример. **README:** [embedding/channels/README.md](embedding/channels/README.md).

Создаёт внутрипроцессную FIFO, отправляет и получает значение, затем уничтожает канал. Это базовый транспортный пример; сопровождающий channel.as описывает отдельный синтаксический эскиз.

**Файлы:** [channel.as](embedding/channels/channel.as), [main.c](embedding/channels/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-file_channels"></a>
#### file_channels

**Тип:** C/C++ — собираемый пример. **README:** [embedding/file_channels/README.md](embedding/file_channels/README.md).

Сериализует объект в файловый канал и восстанавливает его в другом контексте. Показывает владение независимым декодированным графом и возможность читать файл позднее или из другого процесса.

**Файлы:** [main.c](embedding/file_channels/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-resource_broker"></a>
#### resource_broker

**Тип:** C/C++ — собираемый пример. **README:** [embedding/resource_broker/README.md](embedding/resource_broker/README.md).

Экспортирует живой mutex через TCP-брокер, сохраняет ссылку в файловом канале и импортирует её в другом контексте. Восстанавливается доступ к исходному ресурсу, а не снимок его состояния; требуется OS_PORTS.

**Файлы:** [main.c](embedding/resource_broker/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-shared_mutex"></a>
#### shared_mutex

**Тип:** C/C++ — собираемый пример. **README:** [embedding/shared_mutex/README.md](embedding/shared_mutex/README.md).

Использует установленные расширения mutex/channel в нескольких worker-контекстах. Демонстрирует разделяемую блокировку и обмен сериализованными значениями при раздельном владении обычными объектами.

**Файлы:** [main.c](embedding/shared_mutex/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-functions"></a>
### functions

<a id="example-embedding-cpp98_stack"></a>
#### cpp98_stack

**Тип:** C/C++ — собираемый пример. **README:** [embedding/cpp98_stack/README.md](embedding/cpp98_stack/README.md).

Создаёт flashvm::Stack в области видимости C++98, помещает число, читает его по отрицательному индексу и удаляет. Показывает RAII поверх C ABI и необходимость завершить жизнь стека до уничтожения контекста.

**Файлы:** [main.cpp](embedding/cpp98_stack/main.cpp).

[К оглавлению](#оглавление).

<a id="example-embedding-function_parameters"></a>
#### function_parameters

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/function_parameters/README.md](embedding/function_parameters/README.md).

Исходник echo(n) и ссылка на байткодный тест показывают передачу параметра и запись результата в глобальную переменную. Отдельного main нет; исходный AS компилируется внешним инструментом.

**Файлы:** [example.as](embedding/function_parameters/example.as).

[К оглавлению](#оглавление).

<a id="example-embedding-function_rpc"></a>
#### function_rpc

**Тип:** C/C++ — собираемый пример. **README:** [embedding/function_rpc/README.md](embedding/function_rpc/README.md).

Одна native-функция используется через интерфейсы между процессами, независимыми VM-потоками и узлами LAN. Показывает брокер, маршрутизацию вызовов, обслуживание исходного контекста, ошибки и время жизни экспортированной функции.

**Файлы:** [main.c](embedding/function_rpc/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-object_methods"></a>
#### object_methods

**Тип:** C/C++ — собираемый пример. **README:** [embedding/object_methods/README.md](embedding/object_methods/README.md).

Регистрирует native-функцию, связывает её с объектом и вызывает как метод. Позволяет увидеть, как receiver/self и аргументы отличаются от обычного глобального вызова.

**Файлы:** [example.as](embedding/object_methods/example.as), [main.c](embedding/object_methods/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-script_functions"></a>
#### script_functions

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/script_functions/README.md](embedding/script_functions/README.md).

Исходник иллюстрирует объявление и вызов AVM1-функции и получение результата через глобальную переменную. README направляет к отдельному байткодному тесту; текстовый AS-парсер в этот пример не входит.

**Файлы:** [example.as](embedding/script_functions/example.as).

[К оглавлению](#оглавление).

<a id="example-embedding-script_methods"></a>
#### script_methods

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/script_methods/README.md](embedding/script_methods/README.md).

Показывает присваивание функции свойству объекта и вызов через объект. Помогает разобраться с this и методом; это AS-исходник и описание соответствующего регрессионного сценария, а не отдельная C-программа.

**Файлы:** [example.as](embedding/script_methods/example.as).

[К оглавлению](#оглавление).

<a id="group-embedding-gc"></a>
### gc

<a id="example-embedding-cycle_collection"></a>
#### cycle_collection

**Тип:** Заметка — без отдельного main. **README:** [embedding/cycle_collection/README.md](embedding/cycle_collection/README.md).

Концептуальная заметка о сборке взаимно ссылающихся объектов после исчезновения всех корней. Подходит для объяснения отличия трассирующего GC от подсчёта ссылок; самостоятельного исходника здесь нет.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="example-embedding-gc"></a>
#### gc

**Тип:** C/C++ — собираемый пример. **README:** [embedding/gc/README.md](embedding/gc/README.md).

Создаёт объект, закрепляет его C-корнем, запускает сборку, снимает закрепление и собирает снова. Показывает, когда borrowed [FVM_Object](../src/core/runtime.hpp#L121) перестаёт быть допустимым для доступа.

**Файлы:** [main.c](embedding/gc/main.c), [usage.as](embedding/gc/usage.as).

[К оглавлению](#оглавление).

<a id="example-embedding-manual_collection"></a>
#### manual_collection

**Тип:** Заметка — без отдельного main. **README:** [embedding/manual_collection/README.md](embedding/manual_collection/README.md).

Краткая заметка о ручном запуске GC в безопасной точке и закреплении объектов, которые удерживает хост. Это последовательность действий для интеграции, а не отдельная исполняемая программа.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="example-embedding-mode_tuning"></a>
#### mode_tuning

**Тип:** Заметка — без отдельного main. **README:** [embedding/mode_tuning/README.md](embedding/mode_tuning/README.md).

Объясняет выбор режима GC в диапазоне -100..100 и чтение соответствующего порога. Полезна при настройке компромисса между частотой проверок и объёмом удерживаемой памяти; исходника main нет.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="example-embedding-resurrection"></a>
#### resurrection

**Тип:** Заметка — без отдельного main. **README:** [embedding/resurrection/README.md](embedding/resurrection/README.md).

Описывает возвращение объекта в достижимый граф из __gc и повторную маркировку после финализации. Это концептуальная заметка о контракте финализатора, а не пример с отдельным main.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="group-embedding-graphics"></a>
### graphics

<a id="example-embedding-bitmap_fill"></a>
#### bitmap_fill

**Тип:** C/C++ — собираемый пример. **README:** [embedding/bitmap_fill/README.md](embedding/bitmap_fill/README.md).

Создаёт BitmapData из двух пикселей, использует живой bitmap как заливку MovieClip и рендерит результат. Показывает сохранение GC-корней для удерживаемых объектов и порядок освобождения ресурсов.

**Файлы:** [main.c](embedding/bitmap_fill/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-color_matrix_filter"></a>
#### color_matrix_filter

**Тип:** C/C++ — собираемый пример. **README:** [embedding/color_matrix_filter/README.md](embedding/color_matrix_filter/README.md).

Заполняет двадцать коэффициентов ColorMatrixFilter и меняет красный и синий каналы одного RGBA-пикселя. Демонстрирует копирование матрицы и работу фильтра без большого изображения.

**Файлы:** [main.c](embedding/color_matrix_filter/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-drawing"></a>
#### drawing

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/drawing/README.md](embedding/drawing/README.md).

AS2-эскиз рисования и заметка об интеграции drawing API. В папке нет самостоятельного C main; её историческое описание не следует читать как перечень всего современного renderer API.

**Файлы:** [example_as2.as](embedding/drawing/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-embedding-geometry"></a>
#### geometry

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/geometry/README.md](embedding/geometry/README.md).

AS2-исходник использует Point, Matrix и Rectangle: перенос координат, полуоткрытые границы и расширение прямоугольника. Рассчитан на внешний AVM1-компилятор; связан с байткодными тестами геометрии.

**Файлы:** [example_as2.as](embedding/geometry/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-embedding-gradients"></a>
#### gradients

**Тип:** C/C++ — собираемый пример. **README:** [embedding/gradients/README.md](embedding/gradients/README.md).

Создаёт редактируемый прямоугольный путь и линейную красно-синюю заливку, затем печатает центральный RGBA. Показывает матрицу градиента, диапазон stop ratios и владение массивом остановок; графический backend выбирается в DEFS.

**Файлы:** [example_as2.as](embedding/gradients/example_as2.as), [main.c](embedding/gradients/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-image_codecs"></a>
#### image_codecs

**Тип:** C/C++ — собираемый пример. **README:** [embedding/image_codecs/README.md](embedding/image_codecs/README.md).

Разбирает и растеризует SVG и использует подключённую PNG-библиотеку. Демонстрирует обмен буферами изображения и освобождение SVG; доступность зависит от выбранных библиотек и профиля.

**Файлы:** [main.c](embedding/image_codecs/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-media"></a>
#### media

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/media/README.md](embedding/media/README.md).

AS2-эскиз Sound/Video и заметка о подключении медиа backend. В этой папке нет запускаемого C-примера; поддержка конкретных операций проверяется отдельно по документации подсистемы.

**Файлы:** [example_as2.as](embedding/media/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-embedding-movieclip"></a>
#### movieclip

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/movieclip/README.md](embedding/movieclip/README.md).

AS2-эскиз манипуляций MovieClip и заметка о host-интеграции display list. Его назначение — показать идею использования; наличие файла не означает приёмку всех операций таймлайна.

**Файлы:** [example_as2.as](embedding/movieclip/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-embedding-parallel_render"></a>
#### parallel_render

**Тип:** C/C++ — собираемый пример. **README:** [embedding/parallel_render/README.md](embedding/parallel_render/README.md).

Передаёт независимые render jobs исполнителю, используя вызывающий поток для одного задания и native-потоки для остальных. Показывает разделение callback/userdata и синхронное завершение пакета работ.

**Файлы:** [example.as](embedding/parallel_render/example.as), [main.c](embedding/parallel_render/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-stage_quality"></a>

#### stage_quality

**Тип:** C89 — собираемый пример. **README:** [embedding/stage_quality/README.md](embedding/stage_quality/README.md).

Рисует непрозрачный треугольник с дробными координатами в HIGH, выполняет реальный
AVM1 ToggleQuality и повторяет отрисовку в LOW. Печатает количество пикселей с
промежуточной альфой, демонстрируя отличие сглаживания от прозрачности краски.
Использует контекстную политику, без глобального состояния; требует AGG/Cairo,
а NO_GRAPHICS явно сообщает об отключённом renderer.
Именованные `_quality`/`_highquality` демонстрируют запись строкового качества и чтение его числового представления через обычный object API.


**Файлы:** [main.c](embedding/stage_quality/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-input"></a>
### input

<a id="example-embedding-gesture_dispatch"></a>
#### gesture_dispatch

**Тип:** C/C++ — собираемый пример. **README:** [embedding/gesture_dispatch/README.md](embedding/gesture_dispatch/README.md).

Сопоставляет распознанный двухпальцевый tap с правой кнопкой мыши и синхронно передаёт событие C callback. Показывает именно host dispatch; сопровождающий AS-файл является отдельным эскизом.

**Файлы:** [example_as2.as](embedding/gesture_dispatch/example_as2.as), [main.c](embedding/gesture_dispatch/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-gesture_recognizer"></a>
#### gesture_recognizer

**Тип:** C/C++ — собираемый пример. **README:** [embedding/gesture_recognizer/README.md](embedding/gesture_recognizer/README.md).

Подаёт BEGIN/MOVE/END/CANCEL в host-распознаватель и преобразует двухпальцевый tap в кнопку мыши. Объясняет монотонное время, координаты сцены и отличия распознавания от непосредственного AS-события.

**Файлы:** [gesture.as](embedding/gesture_recognizer/gesture.as), [main.c](embedding/gesture_recognizer/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-input"></a>
#### input

**Тип:** C/C++ — собираемый пример. **README:** [embedding/input/README.md](embedding/input/README.md).

Настраивает правила tap/swipe и сопоставляет уже распознанные жесты с мышью и колесом. Подходит для первого знакомства с mapper; сам пример не создаёт оконную систему и не распознаёт сырые касания.

**Файлы:** [example_as2.as](embedding/input/example_as2.as), [main.c](embedding/input/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-input_queue"></a>
#### input_queue

**Тип:** C/C++ — собираемый пример. **README:** [embedding/input_queue/README.md](embedding/input_queue/README.md).

Помещает распознанные события в отложенную очередь и передаёт их callback при pump на VM-потоке. Позволяет отделить получение ввода от его обработки без графического backend.

**Файлы:** [example.as](embedding/input_queue/example.as), [main.c](embedding/input_queue/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-input-keyboard"></a>
#### keyboard

<a id="example-embedding-keyboard_listeners"></a>
##### keyboard_listeners

**Тип:** C89 — собираемый пример. **README:** [embedding/keyboard_listeners/README.md](embedding/keyboard_listeners/README.md).

Регистрирует один объект с native onKeyDown/onKeyUp, публикует полное нажатие и
отпускание без промежуточного кадра, читает callback-visible состояние и снимает
подписку. Показывает сильное владение контекстом, временный GC pin и время жизни
borrowed userdata. Не требует окна или устройства. [Контракт](../doc/KEYBOARD_LISTENERS.ru.md).

Вторая часть выбирает нативный MovieClip через C API, доставляет ещё одно
нажатие/отпускание и сбрасывает выбор. См. [контракт выбора](../doc/KEYBOARD_FOCUS.ru.md).

**Файлы:** [main.c](embedding/keyboard_listeners/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-keyboard_polling"></a>
##### keyboard_polling

**Тип:** C89 — собираемый пример. **README:** [embedding/keyboard_polling/README.md](embedding/keyboard_polling/README.md).

Публикует независимые удержания Left/Right в контексте, вызывает настоящие методы
Key и очищает состояние как при потере фокуса. Не требует устройства или окна;
показывает владение снимком, явную установку объекта и обычную обработку ошибок.
[API и ограничения](../doc/KEYBOARD.ru.md).

**Файлы:** [main.c](embedding/keyboard_polling/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-selection"></a>
##### selection

**Тип:** C89 — собираемый пример. **README:** [embedding/selection/README.md](embedding/selection/README.md).

Сравнивает штатный отказ выбора обычного клипа с успешным выбором после
focusEnabled=true. Показывает явную установку Selection, counted-путь от
getFocus, уведомления onSetFocus/onKillFocus, регистрацию/удаление наблюдателя,
снятие через undefined, construction pin и освобождение
ресурсов при ошибках. Не открывает окна и устройства.
[Контракт EN](../doc/SELECTION.md) / [RU](../doc/SELECTION.ru.md).

**Файлы:** [main.c](embedding/selection/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-mixed_input"></a>
#### mixed_input

**Тип:** исполняемый C89. **README:** [embedding/mixed_input/README.md](embedding/mixed_input/README.md).

Сохраняет контакт и полный клавиатурный снимок с позицией указателя в едином FIFO. Ограничивает пакет доставки, проверяет независимость копии и явно отменяет остаток; сброс polling-состояния выполняется отдельно. Без SDK и графики, не display frontend.

**Файлы:** [main.c](embedding/mixed_input/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-input-mouse"></a>
#### mouse

<a id="example-embedding-mouse_listeners"></a>
##### mouse_listeners

**Тип:** C89 — собираемый пример. **README:** [embedding/mouse_listeners/README.md](embedding/mouse_listeners/README.md).

Регистрирует объект с onMouseDown/onMouseMove/onMouseUp и передаёт три перехода
первичного контакта без промежуточного кадра. Показывает опубликованные координаты,
сильное владение подпиской, временный pin и очистку после ошибки. Подходит для
касания, пера или мыши; не требует устройства или окна.
[Контракт EN](../doc/MOUSE.md) / [RU](../doc/MOUSE.ru.md).

**Файлы:** [main.c](embedding/mouse_listeners/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-jit"></a>
### jit

<a id="example-embedding-execution_modes"></a>
#### execution_modes

**Тип:** C/C++ — собираемый пример. **README:** [embedding/execution_modes/README.md](embedding/execution_modes/README.md).

Задаёт режимы исполнения на входе и во время работы, затем демонстрирует opt-in replay ошибки в интерпретаторе. Показывает счётчики и диагностические callback; повторное исполнение требует учёта побочных эффектов.

**Файлы:** [main.c](embedding/execution_modes/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-jit"></a>
#### jit

**Тип:** C/C++ — собираемый пример. **README:** [embedding/jit/README.md](embedding/jit/README.md).
Показывает ABI сборки; MANUAL и счётчик native entries подтверждают интерпретацию до привязки и реальный JIT после неё.
Darwin ARM64 encoder и включаемый MAP_JIT callback-пул описаны в [JIT_MACOS_ARM64.md](../doc/JIT_MACOS_ARM64.md); opt-in и entitlement требуют отдельной нативной Apple-приёмки.
Руководство также описывает macOS Intel backend, RW→RX, цепочку RBP и необязательную подготовку подписи через entitlement plist.
Руководство описывает Windows x64 backend, W^X/unwind и Linux ms_abi evidence, отдельно отмечая ещё не выполненную нативную Windows приёмку.

Явно компилирует тело x*2+2 и подключает его к обычной AVM1-функции. Сравнивает результат вызова и размер кода, показывает отдельное владение [FVM_JitCode](../src/jit/internal.hpp#L59) и interpreter fallback.

**Файлы:** [main.c](embedding/jit/main.c), [ARM64 entitlements](embedding/jit/macos-arm64.entitlements), [Intel entitlements](embedding/jit/macos-intel.entitlements).

[К оглавлению](#оглавление).

<a id="example-embedding-jit_debug"></a>
#### jit_debug

**Тип:** C/C++ — собираемый пример. **README:** [embedding/jit_debug/README.md](embedding/jit_debug/README.md).

Добавляет ручную source map и breakpoint, затем читает статистику исполнения. Демонстрирует исключение выбранного тела из JIT на следующем входе и синхронную остановку интерпретатора.

**Файлы:** [main.c](embedding/jit_debug/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-jit_instances"></a>
#### jit_instances

**Тип:** C/C++ — собираемый пример. **README:** [embedding/jit_instances/README.md](embedding/jit_instances/README.md).

Создаёт две VM с одинаковым исходным телом, но независимыми режимами и кэшами. Вызовы чередуются на одном потоке; пример показывает изоляцию политик и статистики, а не совместный доступ к объектам.

Опция `--both-jit` требует нативных entries обеих VM; после уничтожения первой пример повторно вызывает вторую и проверяет её счётчики.

**Файлы:** [main.c](embedding/jit_instances/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-jit_workers"></a>
#### jit_workers

**Тип:** C/C++ — собираемый пример. **README:** [embedding/jit_workers/README.md](embedding/jit_workers/README.md).

Запускает два native worker-потока, каждый со своей VM и автоматическим JIT-кэшем. Главный поток получает результаты после join; управляемые объекты и изменяемое script-состояние между VM не разделяются.

**Файлы:** [main.c](embedding/jit_workers/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-objects"></a>
### objects

<a id="example-embedding-main"></a>
#### main

**Тип:** C/C++ — собираемый пример. **README:** [embedding/README.md](embedding/README.md).

Базовая интеграция C89: создание runtime/context, объекта и свойства, публикация глобального значения, регистрация и вызов nativeSum. Показывает исходный жизненный цикл хоста и освобождение контекста раньше runtime.

**Файлы:** [main.c](embedding/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-objects"></a>
#### objects

**Тип:** C/C++ — собираемый пример. **README:** [embedding/objects/README.md](embedding/objects/README.md).

Создаёт объект, записывает и считывает свойства и публикует ссылку глобально. Полезен для изучения [FVM_Value](../include/flashvm/value.h#L24) и контекстного владения; сопровождающий AS2-файл показывает использование объекта со стороны скрипта.

**Файлы:** [example_as2.as](embedding/objects/example_as2.as), [main.c](embedding/objects/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-property_resolve"></a>
#### property_resolve

**Тип:** C/C++ — собираемый пример. **README:** [embedding/property_resolve/README.md](embedding/property_resolve/README.md).

Подключает ленивое разрешение отсутствующих свойств через __resolve и вызывает его через API/AVM1. Показывает receiver, повторное использование функции, GC-корни и отличие отсутствующего свойства от собственного поля.

**Файлы:** [main.c](embedding/property_resolve/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-prototypes"></a>
#### prototypes

**Тип:** C/C++ — собираемый пример. **README:** [embedding/prototypes/README.md](embedding/prototypes/README.md).

Создаёт объект и прототип, связывает их и читает унаследованное свойство. Показывает доступ через цепочку прототипов и раздельное владение объектами внутри одного контекста.

**Файлы:** [example.as](embedding/prototypes/example.as), [main.c](embedding/prototypes/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-standard_boolean"></a>
#### standard_boolean

**Тип:** C/C++ — собираемый пример. **README:** [embedding/standard_boolean/README.md](embedding/standard_boolean/README.md).

Устанавливает стандартные объекты и сравнивает вызов Boolean как функции и конструктора. Демонстрирует преобразование примитива, boxed-объект, методы и защиту удерживаемых объектов от GC.

**Файлы:** [example.as](embedding/standard_boolean/example.as), [main.c](embedding/standard_boolean/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-tables_debug"></a>
#### tables_debug

**Тип:** C/C++ — собираемый пример. **README:** [embedding/tables_debug/README.md](embedding/tables_debug/README.md).

Записывает именованное и индексное поле, читает число имён и количество объектов VM. Минимальный пример сочетания table API с диагностической инспекцией без пользовательских getter.

**Файлы:** [main.c](embedding/tables_debug/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-platform"></a>
### platform

<a id="example-embedding-network"></a>
#### network

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [embedding/network/README.md](embedding/network/README.md).

AS2-эскиз сетевой интеграции и host bridge. Отдельного native main в папке нет; позволяет обсудить прикладную схему, но не заменяет примеры реально подключённых TCP-портов.

**Файлы:** [example_as2.as](embedding/network/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-embedding-os_ports"></a>
#### os_ports

**Тип:** C/C++ — собираемый пример. **README:** [embedding/os_ports/README.md](embedding/os_ports/README.md).

Использует native mouse polling и TCP connect/send/close. Показывает платформенный слой и ресурсное закрытие; наличие успешного TCP-соединения зависит от внешнего адреса и окружения.

**Файлы:** [example.as](embedding/os_ports/example.as), [main.c](embedding/os_ports/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-text"></a>
### text

<a id="example-embedding-font_libraries"></a>
#### font_libraries

**Тип:** C/C++ — собираемый пример. **README:** [embedding/font_libraries/README.md](embedding/font_libraries/README.md).

Читает OpenType fixture, формирует text run, извлекает редактируемые векторные контуры и преобразует их. Показывает границы, правку точек, заливку и маску; fixture позволяет не зависеть от установленных шрифтов.

**Файлы:** [main.c](embedding/font_libraries/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-standard_text"></a>
#### standard_text

**Тип:** C/C++ — собираемый пример. **README:** [embedding/standard_text/README.md](embedding/standard_text/README.md).

Регистрирует шрифт, создаёт TextField и обращается к его свойствам и методам через обычные объекты VM. Извлекает векторный путь; AS-файл отдельно показывает редактирование текста.

**Файлы:** [editing.as](embedding/standard_text/editing.as), [main.c](embedding/standard_text/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-text"></a>
#### text

**Тип:** C/C++ — собираемый пример. **README:** [embedding/text/README.md](embedding/text/README.md).

Проверяет UTF-8 и выполняет поиск регулярным выражением через C API. Позволяет проверить обработку строки до передачи в VM; AS2-исходник рядом относится к отдельному script-сценарию.

**Файлы:** [example.as](embedding/text/example.as), [main.c](embedding/text/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-text_layout"></a>
#### text_layout

**Тип:** C/C++ — собираемый пример. **README:** [embedding/text_layout/README.md](embedding/text_layout/README.md).

Создаёт горизонтальную раскладку текста, запрашивает метрики и строки и извлекает контуры отдельных glyph и всего layout. Показывает преобразование и отрисовку owned-путей и порядок освобождения font/layout/path.

**Файлы:** [main.c](embedding/text_layout/main.c).

[К оглавлению](#оглавление).

<a id="group-embedding-workers"></a>
### workers

<a id="example-embedding-worker_channels"></a>
#### worker_channels

**Тип:** C/C++ — собираемый пример. **README:** [embedding/worker_channels/README.md](embedding/worker_channels/README.md).

Создаёт два FIFO для обмена host↔worker и передаёт строку с ответом о её длине. Показывает неблокирующий probe, закрытие каналов и обязательный join до их уничтожения.

**Файлы:** [main.c](embedding/worker_channels/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-worker_lifecycle"></a>
#### worker_lifecycle

**Тип:** C/C++ — собираемый пример. **README:** [embedding/worker_lifecycle/README.md](embedding/worker_lifecycle/README.md).

Создаёт worker в приостановленном состоянии, читает его state, запускает и получает 42 через join. Позволяет отделить создание потока от разрешения начать работу.

**Файлы:** [main.c](embedding/worker_lifecycle/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-worker_stop"></a>
#### worker_stop

**Тип:** C/C++ — собираемый пример. **README:** [embedding/worker_stop/README.md](embedding/worker_stop/README.md).

Запрашивает кооперативную остановку приостановленного worker и ожидает завершения через join_all. Показывает проверку stop_requested в callback и безопасное завершение без принудительного убийства потока.

**Файлы:** [main.c](embedding/worker_stop/main.c).

[К оглавлению](#оглавление).

<a id="example-embedding-workers"></a>
#### workers

**Тип:** C/C++ — собираемый пример. **README:** [embedding/workers/README.md](embedding/workers/README.md).

Запускает native callback в отдельном OS-потоке с новым runtime/context и копией входного значения. После join читает результат до destroy; userdata и управляемые объекты не становятся автоматически общими.

**Файлы:** [main.c](embedding/workers/main.c), [proposed.as](embedding/workers/proposed.as).

[К оглавлению](#оглавление).

<a id="group-extending"></a>
## extending

<a id="group-extending-callbacks"></a>
### callbacks

<a id="example-extending-callable_objects"></a>
#### callable_objects

**Тип:** C/C++ — собираемый пример. **README:** [extending/callable_objects/README.md](extending/callable_objects/README.md).

Создаёт native callable, сохраняет его в свойстве объекта и вызывает с двумя числами. Callback работает со стеком аргументов и формирует результат; userdata остаётся ответственностью хоста.

**Файлы:** [main.c](extending/callable_objects/main.c).

[К оглавлению](#оглавление).

<a id="example-extending-lua_style"></a>
#### lua_style

**Тип:** C/C++ — собираемый пример. **README:** [extending/lua_style/README.md](extending/lua_style/README.md).

Регистрирует add через Lua-подобный стековый API, читает числовые аргументы и помещает сумму. Показывает компактный способ писать native-функции без совместимости с Lua ABI.

**Файлы:** [main.c](extending/lua_style/main.c).

[К оглавлению](#оглавление).

<a id="example-extending-main"></a>
#### main

**Тип:** C/C++ — собираемый пример. **README:** [extending/README.md](extending/README.md).

Регистрирует makePoint, проверяет два числовых аргумента, создаёт объект x/y и возвращает его C-хосту. Это статическая регистрация native callback, а не загрузчик динамической библиотеки.

**Файлы:** [main.c](extending/main.c).

[К оглавлению](#оглавление).

<a id="example-extending-meta_call"></a>
#### meta_call

**Тип:** Заметка — без отдельного main. **README:** [extending/meta_call/README.md](extending/meta_call/README.md).

README ссылается на tests/meta_call.c: объекты связываются метатаблицей и __call, принимающим таблицу аргументов. Показывает соглашение meta-call; отдельного main в examples нет.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="example-extending-native_functions"></a>
#### native_functions

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [extending/native_functions/README.md](extending/native_functions/README.md).

AS2-эскиз обращения к host-функции и заметка о проектируемой интеграции. Запускаемый пример статической регистрации находится в extending/main.c.

**Файлы:** [example_as2.as](extending/native_functions/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-extending-stack_c"></a>
#### stack_c

**Тип:** C/C++ — собираемый пример. **README:** [extending/stack_c/README.md](extending/stack_c/README.md).

Регистрирует callback со стеком [FVM_Stack](../src/capi/stack_c.cpp#L16), читает аргументы, очищает стек и возвращает сумму. Показывает индексацию, размер стека и правило единственного результата.

**Файлы:** [main.c](extending/stack_c/main.c).

[К оглавлению](#оглавление).

<a id="group-extending-integration"></a>
### integration

<a id="example-extending-channel_worker"></a>
#### channel_worker

**Тип:** C/C++ — собираемый пример. **README:** [extending/channel_worker/README.md](extending/channel_worker/README.md).

Передаёт worker через userdata удерживаемый channel handle, ожидает число и возвращает число плюс один. Показывает явное копирование/release дескриптора и join перед освобождением канала.

**Файлы:** [main.c](extending/channel_worker/main.c).

[К оглавлению](#оглавление).

<a id="example-extending-drawing"></a>
#### drawing

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [extending/drawing/README.md](extending/drawing/README.md).

AS2-эскиз drawing bridge для расширения. В этой папке нет самостоятельной C-программы; конкретный host renderer демонстрируют примеры embedding/graphics в каталоге.

**Файлы:** [example_as2.as](extending/drawing/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-extending-media"></a>
#### media

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [extending/media/README.md](extending/media/README.md).

AS2-эскиз интеграции медиа через расширение. Это исходник и историческая проектная заметка, а не готовый динамический media plugin.

**Файлы:** [example_as2.as](extending/media/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-extending-movieclip"></a>
#### movieclip

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [extending/movieclip/README.md](extending/movieclip/README.md).

AS2-эскиз работы расширения с MovieClip. Показывает ожидаемую прикладную идею; исполняемый native module и отдельный main здесь отсутствуют.

**Файлы:** [example_as2.as](extending/movieclip/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-extending-network"></a>
#### network

**Тип:** ActionScript — исходник/эскиз для внешнего компилятора. **README:** [extending/network/README.md](extending/network/README.md).

AS2-эскиз сетевого host bridge для расширений. Не выполняет реального сетевого подключения сам по себе; для native TCP следует смотреть embedding/os_ports.

**Файлы:** [example_as2.as](extending/network/example_as2.as).

[К оглавлению](#оглавление).

<a id="example-extending-prototypes"></a>
#### prototypes

**Тип:** Заметка — без отдельного main. **README:** [extending/prototypes/README.md](extending/prototypes/README.md).

Заметка о построении native-объектов на прототипах и ссылка на embedding/prototypes. Это описание повторного использования C ABI, без отдельной реализации загружаемого плагина.

**Файлы:** отдельного исходника нет; руководство и связанные API перечислены в README.

[К оглавлению](#оглавление).

<a id="example-extending-sqlite"></a>
#### sqlite

**Тип:** C/C++ — собираемый пример. **README:** [extending/sqlite/README.md](extending/sqlite/README.md).

Открывает SQLite в памяти, выполняет SQL и получает целочисленный результат. Показывает владение adapter handle; доступность определяется поддержкой SQLite, а AS-файл описывает отдельный host bridge.

**Файлы:** [example_as2.as](extending/sqlite/example_as2.as), [main.c](extending/sqlite/main.c).

[К оглавлению](#оглавление).

<a id="group-frontends"></a>
## frontends

[Общее руководство и FlashVM API](frontends/README.md). Самостоятельные хосты вывода/ввода; общая часть common/demo.h — не отдельный frontend. Headless перенесён из embedding/host_backend; его CMake-target теперь flashvm_examples_frontends_headless_main. Других самостоятельных фронтендов в примерах не найдено.

<a id="group-frontends-memory"></a>
### memory

<a id="example-frontends-headless"></a>
#### headless

**Тип:** исполняемый C89. **README:** [frontends/headless/README.md](frontends/headless/README.md).

Рисует реальный квадрат, передаёт RGBA-кадр синхронному consumer и проверяет мировые координаты down/up в VM. Не требует оконного SDK; при NO_GRAPHICS явно пропускается.

**Файлы:** [main.c](frontends/headless/main.c).

[К оглавлению](#оглавление).

<a id="group-frontends-windows"></a>
### windows

<a id="example-frontends-clutter3d"></a>
#### clutter3d

**Тип:** C89 с явным SDK выбранного Clutter/Cogl Git. **README:** [frontends/clutter3d/README.md](frontends/clutter3d/README.md).

Загружает RGBA в texture actor, переводит stage→actor→world, выбирает первый touch sequence и отменяет контакт при деактивации. Исходники SDK берутся из bhgv/Inferno-OS_Android-Clutter3d без inferno-os; отдельно описана интеграция с Android-ready/surface lifecycle. Не готовый APK.

**Файлы:** [main.c](frontends/clutter3d/main.c), [общий demo.h](frontends/common/demo.h).

[К оглавлению](#оглавление).

<a id="example-frontends-sdl"></a>
#### sdl

**Тип:** исполняемый C89 с явным SDL2 SDK. **README:** [frontends/sdl/README.md](frontends/sdl/README.md).

Показывает marker из VM через настоящую streaming-текстуру SDL. Обрабатывает мышь и первое касание с подавлением синтетических дублей, сбросом фокуса и логическими координатами. Есть интерактивный и конечный smoke-режимы.

**Файлы:** [main.c](frontends/sdl/main.c), [общий demo.h](frontends/common/demo.h).

[К оглавлению](#оглавление).

<a id="group-multimedia"></a>
## multimedia

<a id="group-multimedia-bridges"></a>
### bridges

<a id="example-multimedia-alsa_output"></a>
#### alsa_output

Linux ALSA: явное устройство, короткий тон и StopSounds. Исполняемый C89 учебник:
[README](multimedia/alsa_output/README.md), [main.c](multimedia/alsa_output/main.c).
Provider использует статическую ALSA, channel копирует реальные callbacks;
Enter в host-программе вызывает остановку очереди через Action 0x09.
Показывает S16/rate/channels/latency, nonblocking write/drop/reprepare и порядок
context → provider cleanup. Требует явного endpoint и предварительной подготовки
SDK/прав/плагинов. Null PCM проверяет библиотеку, но не слышимый звук.

<a id="example-multimedia-audio_channels"></a>
#### audio_channels

Регистрация PCM-выхода и StopSounds. Исполняемый C89 учебник:
[README](multimedia/audio_channels/README.md), [main.c](multimedia/audio_channels/main.c).
Два silent mono S16 sample frames записываются через настоящий fwrite в явно
заданный raw PCM файл; Action 0x09 вызывает stop callback, закрывающий FILE.
Показывает inactive/active состояние, копирование таблицы, borrowed буфер и
host userdata, живущий до конца context cleanup. Ошибки обрабатываются обычно,
без assert. Это файловый sink, не звуковая карта; NO_MEDIA поддерживается.

<a id="example-multimedia-custom_audio_bridge"></a>
#### custom_audio_bridge

Свой источник PCM через stdio. Исполняемый C89 учебник: [README](multimedia/custom_audio_bridge/README.md), [main.c](multimedia/custom_audio_bridge/main.c).

Реальный custom backend читает явно заданный raw PCM файл через fopen/fread.
Это пример замены FFmpeg на собственную библиотеку/системный источник, а не
воспроизведение на звуковую карту. Формат: mono, 8000 Hz, native-endian signed
16-bit; заголовка нет. Неполный последний sample считается ошибкой. Backend
реализует open/read/close, оставляет seek null. У каждого session собственный FILE
и буфер. Ошибка open освобождает частичное состояние сама; успешный close закрывает
FILE и освобождает malloc. В session копируется таблица, не userdata.

<a id="example-multimedia-custom_video_output"></a>
#### custom_video_output

Свой вывод RGBA в реальный PPM файл. Исполняемый C89 учебник: [README](multimedia/custom_video_output/README.md), [main.c](multimedia/custom_video_output/main.c).

Показывает написание [FVM_MediaOutput](../include/flashvm/multimedia.h#L68).write поверх реальных fprintf/fwrite.
Стековый RGBA8 кадр 2×1 содержит красный и зелёный пиксели. [fvm_video_present](../include/flashvm/multimedia.h#L137)
проверяет размер/stride; callback пишет PPM P6 и отбрасывает alpha. Файл создаётся
только по явному аргументу; существующий файл по этому пути заменяется. Временный
буфер не сохраняется после возврата callback. FILE принадлежит main и закрывается
даже при неуспешной записи; проверяется также ошибка flush/close.

<a id="example-multimedia-stop_binding"></a>
#### stop_binding

Привязка собственного подготовленного native-аудио к stopAllSounds контекста.
Исполняемый C89 пример без устройства: [README](multimedia/stop_binding/README.md),
[main.c](multimedia/stop_binding/main.c). `--demo` показывает `pending=0 stops=1`.
Объясняет borrowed userdata, detach до освобождения и очистку без первой записи PCM.
Функции API с индивидуальными описаниями перечислены в README;
[контракт](../doc/AUDIO_STOP_BINDINGS.ru.md). Каждая строка подписана, assertions нет.

<a id="group-multimedia-decoding"></a>
### decoding

<a id="example-multimedia-ffmpeg_decode"></a>
#### ffmpeg_decode

FFmpeg: декодирование файлов и явная камера. Исполняемый C89 учебник: [README](multimedia/ffmpeg_decode/README.md), [main.c](multimedia/ffmpeg_decode/main.c).

Исполняемый пример вызывает настоящий backend из статических библиотек FFmpeg.
Режим audio выбирает один аудиопоток и возвращает S16 PCM; video — один видеопоток
и RGBA. Другие потоки пропускаются, временная шкала/синхронизация A/V не реализованы.
При первом буфере печатаются реальные формат/размер; в конце число буферов.
Воспроизведение/окно не создаются. Без MEDIA явный запуск сообщает UNSUPPORTED и
показывает настройку SDK; без аргументов выводится usage с успешным smoke exit.

<a id="example-multimedia-swf_audio"></a>
#### swf_audio

DefineSound: обзор корневых ресурсов и явный экспорт PCM. Исполняемый C89 учебник:
[README](multimedia/swf_audio/README.md), [main.c](multimedia/swf_audio/main.c).
Загружает явно названный FWS/CWS, перечисляет звуковые ID/метаданные, StartSound
с SOUNDINFO/огибающими (без исполнения), затем отправляет
выбранный 8/16-bit PCM или ADPCM с преобразованием в S16 в настоящий stdio sink кусками по 256 sample frames.
Показывает canonical ownership, borrowed views, bounded copy до host callback,
ошибки кодеков/размеров и StopSounds, закрывающий FILE. Нужен явный вход/ID/выход;
NO_MEDIA работает без SDK. Режим `--event` применяет диапазон/повторы/огибающую
одного StartSound одним буфером; обычный режим сохраняет source chunks.
Режим `--stream-event` хранит источник/огибающую один раз и читает повторы
порциями без новых аллокаций: [constants](multimedia/swf_audio/example_constants.h).
Это экспорт raw PCM, не автоматический SWF-плеер.

<a id="example-multimedia-swf_stream"></a>
#### swf_stream

Исполняемый C89 учебник: [README](multimedia/swf_stream/README.md), включая `--prepared`: независимый индекс PCM после уничтожения исходной VM.
[main.c](multimedia/swf_stream/main.c), [constants](multimedia/swf_stream/example_constants.h).
Читает встроенный SoundStreamHead2 и блоки с фактическими номерами кадров,
variable-length stereo PCM, пропуском и явным empty block. Показывает отличие
source geometry от advisory playback и среднего числа samples. Порционные
копии caller buffer уходят в настоящий fopen/fwrite/fclose bridge; stop-all
закрывает output. Файл задаётся явно, без аргументов только usage. NO_MEDIA
работает без SDK; пример не вставляет тишину, не синхронизирует clock, не
декодирует compressed streams и не исполняет MovieClip timeline. Контракты:
[SWF_STREAM_SOUND.md](../doc/SWF_STREAM_SOUND.md).

<a id="example-multimedia-swf_stream_mp3"></a>
#### swf_stream_mp3

Исполняемый C89 пример: [README](multimedia/swf_stream_mp3/README.md),
[main.c](multimedia/swf_stream_mp3/main.c). Проверяет live MP3-префикс,
копирует закодированный сегмент и читает два блока после уничтожения VM.
Показывает различие объявленного SampleCount, signed SeekSamples и header latency,
а также полный префикс с пустым MPEG-телом. Каждая строка подписана, assertions нет.
Синтетические MPEG-байты не являются воспроизводимым аудио; NO_MEDIA достаточен.
Контракты и ограничения: [SWF_STREAM_MP3.md](../doc/SWF_STREAM_MP3.md).

<a id="group-multimedia-devices"></a>
### devices

<a id="example-multimedia-system_devices"></a>
#### system_devices

Свой опрос устройств через Linux evdev. Исполняемый C89 учебник: [README](multimedia/system_devices/README.md), [main.c](multimedia/system_devices/main.c).

Показывает адаптер реальной системной очереди к [FVM_DeviceBridge](../include/flashvm/multimedia.h#L102).poll. Main явно
открывает указанный /dev/input/eventN с O_RDONLY|O_NONBLOCK, хранит fd и передаёт
его адрес через userdata. Callback не владеет fd и делегирует read нативному
переходнику. После максимум 64 polling calls main закрывает fd и проверяет close.
Нет ожидания, фонового потока, изменения прав или поиска устройств.

<a id="group-multimedia-mixing"></a>
### mixing

<a id="example-multimedia-swf_audio_queue"></a>
#### swf_audio_queue

v0.587: явная настройка необязательного хвоста и release stop на модели тишины; [контракт](../doc/PLAYER_AUDIO_RELEASE.ru.md).

Исполняемый C89 учебник: [README](multimedia/swf_audio_queue/README.md),
[main.c](multimedia/swf_audio_queue/main.c). Собственная модель FIFO показывает
отклонение/повтор одного PCM-блока, pause/clear, копируемые часы потребления и
отдельное владение producer/mixer/device. Явный `--sdl` использует настоящий SDL2
bridge при включённом SDK; модель работает без устройства. Тишина и dummy-driver
проверяют протокол, не слышимый вывод. README описывает подготовку Linux/Windows/
macOS, ограничения Android и индивидуальные контракты функций. См.
[EN](../doc/SWF_SOUND_OUTPUT.md) / [RU](../doc/SWF_SOUND_OUTPUT.ru.md).

<a id="example-multimedia-swf_event_mixer"></a>
#### swf_event_mixer

Исполняемый C89 учебник: [README](multimedia/swf_event_mixer/README.md),
[main.c](multimedia/swf_event_mixer/main.c),
[constants](multimedia/swf_event_mixer/example_constants.h).
Создаёт три независимых голоса из встроенного SWF, подавляет повторный
SyncNoMultiple, суммирует с единственным насыщением, останавливает оба голоса
одного ID и продолжает читать другой после уничтожения исходной VM.
Без выходного пути работает в памяти; с явным путём пишет настоящий native S16
PCM через fopen/fwrite/fclose. Каждая строка кода подписана; assert нет.
--timeline-demo доставляет четыре StartSound из реального входа в кадр через
host callback и pin-ит root; оба режима проверяются по точному PCM файлу.
--convert-demo явно преобразует source в 48000 Hz stereo с линейной интерполяцией;
проверяются точные stereo PCM байты и владение после уничтожения VM.
Автоматического device clock нет.
Подробные контракты: [SWF_SOUND_MIXER.md](../doc/SWF_SOUND_MIXER.md).

<a id="example-multimedia-swf_mix"></a>
#### swf_mix

Два подготовленных голоса из встроенного SWF читаются малыми синхронными
порциями и суммируются в один настоящий PCM файл через stdio bridge.
Исполняемый C89 учебник: [README](multimedia/swf_mix/README.md),
[main.c](multimedia/swf_mix/main.c), [constants](multimedia/swf_mix/example_constants.h).
После EOF короткого голоса его tail — тишина; второй голос продолжает повтор.
Останавливает настоящий FILE через AVM1 StopSounds в interpreter либо automatic
JIT; native path проверяет фактический вход в машинный код.
Показывает native ownership, одинаковую geometry, caller buffers, точную сумму
и final-only saturation. NO_MEDIA без SDK; явный output path может быть перезаписан.
Это ручной block pipeline, без автоматического clock, resampling или audible output.

<a id="example-multimedia-swf_polyphony"></a>
#### swf_polyphony

Несколько голосов одного DefineSound: независимые реальные FILE sinks,
context-wide no-multiple и остановка всех голосов по sound ID при сохранении
другого ресурса. Исполняемый C89 учебник: [README](multimedia/swf_polyphony/README.md),
[main.c](multimedia/swf_polyphony/main.c), [constants](multimedia/swf_polyphony/example_constants.h).
Явно выбираются три output path, каждый может быть перезаписан. No-multiple не
открывает и не закрепляет пропущенный канал; затем тот принимает другой ID.
Stop-by-ID закрывает два совпадения, stop_all закрывает оставшееся. NO_MEDIA
достаточен; это stdio/синхронизация, не слышимое смешивание/автоматический clock.

<a id="group-multimedia-speech"></a>
### speech

<a id="example-multimedia-speech_android"></a>
#### speech_android

Android: Context, permissions, JNI и речевые службы. Native app integration source; не standalone executable: [README](multimedia/speech_android/README.md).
[SocketNames.java](multimedia/speech_android/SocketNames.java) показывает реальные Android ABSTRACT/FILESYSTEM соединения.

Исходники интеграции **в реальное Android приложение**, не desktop executable
и не готовый APK. bridge.cpp содержит JNI host с двумя независимыми sessions,
SpeechHost.java — разрешения и Java entrypoints. Настоящие system helpers находятся
в src/multimedia/speech/tts/FlashVMTts.java и stt/FlashVMStt.java.

<a id="example-multimedia-speech_macos"></a>
#### speech_macos

macOS: реальные speech и microphone permissions. Native app integration source; не standalone executable: [README](multimedia/speech_macos/README.md).

Исходник prepare.mm для **настоящего macOS app host** (10.15+), а не переносимый
standalone executable. Он показывает то, что библиотека не должна делать скрыто:
реальные SFSpeechRecognizer/AVCaptureDevice permission prompts после явного
пользовательского действия, ожидание результата, затем open на main queue.

<a id="example-multimedia-speech_stt"></a>
#### speech_stt

Системный STT: разрешения и захват настоящей речи. Исполняемый C89 пример: [README](multimedia/speech_stt/README.md).

Исполняемый C89 host: явно открывает настоящий микрофон/recognizer, до примерно
30 секунд опрашивает actual partial/final текст, закрывается после final либо
ошибки/timeout. Ни одна заранее заданная строка не выдаётся за распознавание.
Без аргументов только usage, без capture/permission dialogs. [GC:] Микрофон,
модель и host transcript allocations не принадлежат VM collector.

<a id="example-multimedia-speech_tts"></a>
#### speech_tts

Системный TTS: подготовка голоса и реальная речь. Исполняемый C89 пример: [README](multimedia/speech_tts/README.md).

Исполняемый C89 пример: заводит настоящий голос/SSIP session, отправляет явный
UTF-8 текст, ждёт Enter, затем останавливает и закрывает session. Это ожидание
нужно: немедленный close отменил бы только что поставленную речь. Без аргументов
пример выводит usage и ничего не произносит/не подключает. Обычные ошибки вместо
assertions; [GC:] native sessions не входят в VM heap и освобождаются явно.

<a id="group-player"></a>
## player

<a id="example-player-animation_audio"></a>
### animation_audio

v0.590: ролик 02 содержит одноразовую регистрацию Key observer, считает оба перехода и показывает их зелёным маркером; [слушатели и очередь](../doc/KEYBOARD_LISTENERS.ru.md).

v0.588: исправлены границы блоков тестового ADPCM-тона; [измерения и проверка](../doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md#v0588-границы-блоков-тестового-adpcm-тона).

v0.587: пяти-миллисекундное затухание явных остановок; [границы и ограничения](../doc/PLAYER_AUDIO_RELEASE.ru.md).

v0.586: [размер системного аудиоблока и темп PCM](../doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md#v0586-публикация-согласованного-блока-устройства) — повторная проверка 01/03 в обоих режимах.

v0.584: [native PCM/WAV и Android ZIP-отчёт](../doc/PLAYER_AUDIO_DIAGNOSTICS.ru.md) для сравнения interpreter/JIT.

Три готовых исполняемых SWF8: [README](player/animation_audio/README.md),
[генератор Python3](player/animation_audio/make_movies.py),
[animation](player/animation_audio/01-animation-audio.swf),
[input](player/animation_audio/02-input-audio.swf),
[StopSounds](player/animation_audio/03-stop-sounds.swf). Третий отменяет часть звуков через AVM1 StopSounds. Первый проверяет timeline
движение; второй — AVM1 управление MovieClip через HostInput, стрелки/Space,
указатель и сохранённые счётчики. Оба содержат тихие U8 PCM/S16 stereo/ADPCM тоны,
loops/no-multiple/stop по ID. Пакеты Linux/Windows и Android menu/SDLActivity
запускают те же файлы в interpreter/JIT. Это не полная совместимость Key/Mouse
или аппаратная приёмка всех ОС; [контракты и сборка](../doc/PLAYER_TEST_BUILDS.md).

С v0.597 --mute выполняет все три звуковых ролика без аудиоустройства; реальный
CLI проверяет 120 кадров каждого в interpreter/JIT.
[Контракт](../doc/PLAYER_POINTER.ru.md#ролики-без-звука).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:102](../include/flashvm/multimedia.h#L102) |
| `fvm_extensions_install` | function | [include/flashvm/extensions.h:17](../include/flashvm/extensions.h#L17) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../src/jit/internal.hpp#L59) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:68](../include/flashvm/multimedia.h#L68) |
| `FVM_Object` | struct | [src/core/runtime.hpp:121](../src/core/runtime.hpp#L121) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:16](../src/capi/stack_c.cpp#L16) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `fvm_video_present` | function | [include/flashvm/multimedia.h:137](../include/flashvm/multimedia.h#L137) |
<!-- source-links:end -->

<a id="glossary"></a>
## Глоссарий — алфавитный список всех примеров

Один список без тематических подгрупп. Сортировка по имени примера; одинаковые
имена различаются путём. Это указатель примеров, а не словарь терминов.

- [alsa_output — multimedia/alsa_output](#example-multimedia-alsa_output)
- [animation_audio — player/animation_audio](#example-player-animation_audio)
- [audio_channels — multimedia/audio_channels](#example-multimedia-audio_channels)
- [backtrace — debugger/backtrace](#example-debugger-backtrace)
- [bitmap_fill — embedding/bitmap_fill](#example-embedding-bitmap_fill)
- [callable_objects — extending/callable_objects](#example-extending-callable_objects)
- [channel_cpp98 — embedding/channel_cpp98](#example-embedding-channel_cpp98)
- [channel_inspection — debugger/channel_inspection](#example-debugger-channel_inspection)
- [channel_values — embedding/channel_values](#example-embedding-channel_values)
- [channel_worker — extending/channel_worker](#example-extending-channel_worker)
- [channels — embedding/channels](#example-embedding-channels)
- [clutter3d — frontends/clutter3d](#example-frontends-clutter3d)
- [color_matrix_filter — embedding/color_matrix_filter](#example-embedding-color_matrix_filter)
- [cpp98_stack — embedding/cpp98_stack](#example-embedding-cpp98_stack)
- [custom_audio_bridge — multimedia/custom_audio_bridge](#example-multimedia-custom_audio_bridge)
- [custom_video_output — multimedia/custom_video_output](#example-multimedia-custom_video_output)
- [cycle_collection — embedding/cycle_collection](#example-embedding-cycle_collection)
- [drawing — embedding/drawing](#example-embedding-drawing)
- [drawing — extending/drawing](#example-extending-drawing)
- [error.as — actionscript/error.as](#example-actionscript-error-as)
- [execution_modes — embedding/execution_modes](#example-embedding-execution_modes)
- [ffmpeg_decode — multimedia/ffmpeg_decode](#example-multimedia-ffmpeg_decode)
- [file_channels — embedding/file_channels](#example-embedding-file_channels)
- [file_channels.as — actionscript/file_channels.as](#example-actionscript-file_channels-as)
- [font_libraries — embedding/font_libraries](#example-embedding-font_libraries)
- [function_parameters — embedding/function_parameters](#example-embedding-function_parameters)
- [function_rpc — embedding/function_rpc](#example-embedding-function_rpc)
- [gc — embedding/gc](#example-embedding-gc)
- [gc_workers_channels.as — actionscript/gc_workers_channels.as](#example-actionscript-gc_workers_channels-as)
- [geometry — embedding/geometry](#example-embedding-geometry)
- [gesture_dispatch — embedding/gesture_dispatch](#example-embedding-gesture_dispatch)
- [gesture_recognizer — embedding/gesture_recognizer](#example-embedding-gesture_recognizer)
- [gradients — embedding/gradients](#example-embedding-gradients)
- [headless — frontends/headless](#example-frontends-headless)
- [image_codecs — embedding/image_codecs](#example-embedding-image_codecs)
- [input — embedding/input](#example-embedding-input)
- [input_queue — embedding/input_queue](#example-embedding-input_queue)
- [jit — embedding/jit](#example-embedding-jit)
- [jit_debug — embedding/jit_debug](#example-embedding-jit_debug)
- [jit_dump — debugger/jit_dump](#example-debugger-jit_dump)
- [jit_instances — embedding/jit_instances](#example-embedding-jit_instances)
- [jit_workers — embedding/jit_workers](#example-embedding-jit_workers)
- [keyboard_listeners — embedding/keyboard_listeners](#example-embedding-keyboard_listeners)
- [keyboard_polling — embedding/keyboard_polling](#example-embedding-keyboard_polling)
- [live_variables — debugger/live_variables](#example-debugger-live_variables)
- [lua_style — extending/lua_style](#example-extending-lua_style)
- [main — embedding/main](#example-embedding-main)
- [main — extending/main](#example-extending-main)
- [manual_collection — embedding/manual_collection](#example-embedding-manual_collection)
- [media — embedding/media](#example-embedding-media)
- [media — extending/media](#example-extending-media)
- [meta_call — extending/meta_call](#example-extending-meta_call)
- [mixed_input — embedding/mixed_input](#example-embedding-mixed_input)
- [mode_tuning — embedding/mode_tuning](#example-embedding-mode_tuning)
- [mouse_listeners — embedding/mouse_listeners](#example-embedding-mouse_listeners)
- [movieclip — embedding/movieclip](#example-embedding-movieclip)
- [movieclip — extending/movieclip](#example-extending-movieclip)
- [native_functions — extending/native_functions](#example-extending-native_functions)
- [network — embedding/network](#example-embedding-network)
- [network — extending/network](#example-extending-network)
- [object_methods — embedding/object_methods](#example-embedding-object_methods)
- [objects — embedding/objects](#example-embedding-objects)
- [os_ports — embedding/os_ports](#example-embedding-os_ports)
- [parallel_render — embedding/parallel_render](#example-embedding-parallel_render)
- [parsing.as — actionscript/parsing.as](#example-actionscript-parsing-as)
- [property_resolve — embedding/property_resolve](#example-embedding-property_resolve)
- [prototypes — embedding/prototypes](#example-embedding-prototypes)
- [prototypes — extending/prototypes](#example-extending-prototypes)
- [resource_broker — embedding/resource_broker](#example-embedding-resource_broker)
- [resource_exceptions.as — actionscript/resource_exceptions.as](#example-actionscript-resource_exceptions-as)
- [resource_snapshots — debugger/resource_snapshots](#example-debugger-resource_snapshots)
- [resurrection — embedding/resurrection](#example-embedding-resurrection)
- [script_functions — embedding/script_functions](#example-embedding-script_functions)
- [script_methods — embedding/script_methods](#example-embedding-script_methods)
- [sdl — frontends/sdl](#example-frontends-sdl)
- [selection — embedding/selection](#example-embedding-selection)
- [shared_mutex — embedding/shared_mutex](#example-embedding-shared_mutex)
- [shared_mutex.as — actionscript/shared_mutex.as](#example-actionscript-shared_mutex-as)
- [source_breakpoints — debugger/source_breakpoints](#example-debugger-source_breakpoints)
- [speech_android — multimedia/speech_android](#example-multimedia-speech_android)
- [speech_macos — multimedia/speech_macos](#example-multimedia-speech_macos)
- [speech_stt — multimedia/speech_stt](#example-multimedia-speech_stt)
- [speech_tts — multimedia/speech_tts](#example-multimedia-speech_tts)
- [sprite_resources.as — actionscript/sprite_resources.as](#example-actionscript-sprite_resources-as)
- [sqlite — extending/sqlite](#example-extending-sqlite)
- [stack_c — extending/stack_c](#example-extending-stack_c)
- [stage_quality — embedding/stage_quality](#example-embedding-stage_quality)
- [standard_boolean — embedding/standard_boolean](#example-embedding-standard_boolean)
- [standard_text — embedding/standard_text](#example-embedding-standard_text)
- [stop_binding — multimedia/stop_binding](#example-multimedia-stop_binding)
- [string.as — actionscript/string.as](#example-actionscript-string-as)
- [swf_audio — multimedia/swf_audio](#example-multimedia-swf_audio)
- [swf_audio_queue — multimedia/swf_audio_queue](#example-multimedia-swf_audio_queue)
- [swf_event_mixer — multimedia/swf_event_mixer](#example-multimedia-swf_event_mixer)
- [swf_mix — multimedia/swf_mix](#example-multimedia-swf_mix)
- [swf_polyphony — multimedia/swf_polyphony](#example-multimedia-swf_polyphony)
- [swf_stream — multimedia/swf_stream](#example-multimedia-swf_stream)
- [swf_stream_mp3 — multimedia/swf_stream_mp3](#example-multimedia-swf_stream_mp3)
- [system_devices — multimedia/system_devices](#example-multimedia-system_devices)
- [tables_debug — embedding/tables_debug](#example-embedding-tables_debug)
- [text — embedding/text](#example-embedding-text)
- [text_layout — embedding/text_layout](#example-embedding-text_layout)
- [worker_channels — embedding/worker_channels](#example-embedding-worker_channels)
- [worker_lifecycle — embedding/worker_lifecycle](#example-embedding-worker_lifecycle)
- [worker_stop — embedding/worker_stop](#example-embedding-worker_stop)
- [workers — embedding/workers](#example-embedding-workers)

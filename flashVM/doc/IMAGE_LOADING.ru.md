# Атомарная загрузка изображений в MovieClip

[English](IMAGE_LOADING.md) · [Источники](PLAYER_SOURCES.ru.md) ·
[Очистка](MOVIE_UNLOAD.ru.md) · [Роудмап](ROADMAP_STATUS.md).

## C API с v0.700

`fvm_movieclip_replace_image(context, clip, bytes, size, source, source_size)`
декодирует целый PNG, статический полноразмерный GIF89a или RGB/grayscale JPEG
в BitmapData straight RGBA и один остановленный кадр с прямоугольником.
Сохраняются экземпляр, прототип, размещение, собственные свойства, обработчики
и lockroot. Старые дети отсоединяются с сохранением прежнего origin; совпадающие
именные связи удаляются. Новая база источника копируется. Конструкторы, getters,
setters и действия кадров не вызываются.

Только поток владельца и безопасная точка вне AVM1/native callbacks, sound delivery,
GC и уничтожения. Контекст/цель удерживаются до возврата; для ссылок через последующие
собирающие вызовы нужен pin. Входные bytes/source заимствуются неизменяемыми и не
освобождаются библиотекой. Временный bitmap/drawing graph защищён native GC barrier.
Проверки, декодирование и выделения графа предшествуют публикации. Ошибка сохраняет
прежние содержимое, рисование и связи детей. Успех запрашивает stage update.

PNG/GIF89a и progressive JPEG требуют исходного SWF8, baseline JPEG — SWF6.
Native C-клипы без SWF-профиля принимают поддержанные изображения. Старый контракт
декодера embedded JPEG не меняется. Формат определяется сигнатурой, не расширением
или HTTP MIME. Неподдержанный формат/профиль/сборка, GIF87a, animated/partial-canvas
GIF и неподдержанное цветовое пространство JPEG дают [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).
Повреждённый распознанный поток и неверные аргументы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45),
отказ памяти — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), прочие ошибки реализации — [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).

Вход и отдельный RGBA payload ограничены [`FVM_SWF_DEFAULT_LIMIT`](../include/flashvm/swf.h#L15), размеры —
2880 × 2880. Непустой counted source не содержит NUL и не превышает
[`FVM_MOVIE_SOURCE_MAX_BYTES`](../include/flashvm/movieclip.h#L95). Несколько временных копий увеличивают пиковую память;
это не общий process memory quota. Память C-декодера освобождается и при C++ exception.

[`fvm_movieclip_replace_resource`](../include/flashvm/movieclip.h#L129) выбирает SWF или изображение по сигнатуре;
повреждённый SWF не переходит в image fallback. [`fvm_movieclip_replace_swf`](../include/flashvm/movieclip.h#L114) остаётся
только для SWF. Публичные структуры и callbacks не менялись. Интеграция с очередью
плеера описана ниже.

## Сборка и проверки

Внешние codecs — приложенные статические libpng/giflib/libjpeg и существующие
support libraries. Команды и профиль `cmake/profiles/player-images.defs` приведены
в [English](IMAGE_LOADING.md#build-and-tests). SDL, сеть и AS-компилятор для API
не нужны. Сборка NO_IMAGE_CODECS возвращает явное UNSUPPORTED. Не сочетать такой
definitions file с противоположным CMake override: выбирается соответствующий файл.
Профиль включает JIT с обычной проверкой доступности на целевом CPU.

Проверяются PNG/GIF alpha, JPEG pixels, идентичность, собственные свойства,
отсоединённые origin/GC, повреждённый вход, animated GIF, SWF/image dispatch и
baseline/progressive JPEG при SWF7. Перебор C++ new/new[] отказов проверяет старый
граф и связи. Он не заменяет отдельные проверки C-декодеров и их allocator paths.
## Доставка дочерних изображений плеером с v0.701

Дочерний loadMovie теперь вызывает общий resource API для local, synchronous,
background-waiting и async. Сокеты HTTP/TLS и libcurl сохраняют существующие policy,
deadline, monitor, cancel и provenance. Worker обрабатывает только bytes; decoder,
import и публикация выполняются владельцем VM. Очередь с pin и отмена через unload
защищают и изображения. MIME и расширение не определяют доверие к формату.

Ширина/высота доступны через обычные индексы ActionGetProperty. Однокадровое
изображение остановлено и не исполняет source actions. Тот же экземпляр можно
очистить и загрузить в него SWF с сохранением свойств/размещения. Старт и замена
корня всё ещё требуют SWF: root image/window sizing и loader events не реализованы.

Реальные PNG/GIF/JPEG проверяют local/waiting/async, opaque MIME и .bin URL,
размеры, удаление детей, идентичность/свойства, unload затем SWF reload, повреждённый
ответ и TLS с явным CA. Сборка без codecs не выдаёт принятие изображения за успех.
[Core C89 tutorial](../examples/embedding/image_codecs_loading/README.md) / [RU](../examples/embedding/image_codecs_loading/README.ru.md); [player C/AS/SWF tutorial](../examples/player/image_loading/README.md) / [RU](../examples/player/image_loading/README.ru.md).

## Переносимая сборка с изображениями

Выберите `cmake/profiles/player-images.defs` вместе с
`-DFVM_ENABLE_IMAGE_CODECS=ON`: примеры выше содержат команды нативной и
кросс-сборки с явными путями SDK. Кодеки этого профиля линкуются статически.
Интерфейсы и хосты FlashVM сохраняют C89, VM — C++98. Для внешних кодеков,
pixman и Cairo локально выбран C99; только Cairo под MinGW использует GNU99
для объявления старых функций Windows CRT. Размеры `int`, `long`, `long long`
определяются целевым компилятором без запуска целевого кода, включая LLP64 Windows.

Серия v0.700–v0.703 прошла 40 выбранных нативных тестов, 15 в конфигурации
разделяемой библиотеки с socket-TLS и 13 с curl. Ранее в этой же серии прошли
семь проверок изображений без сетевого провайдера и две проверки без кодеков.
Плеер и оба C-примера скомпонованы для Linux ARM64, Linux RISC-V64 и Windows
x86-64 со звуком; целевые программы не запускались. UBSan для C-кода
player/transfer/audio проверяет интерпретатор/JIT и пример с URL/встроенным
буфером; VM, кодеки, SDL и транспорт в этой проверке не инструментированы.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOVIE_SOURCE_MAX_BYTES` | macro / define | [include/flashvm/movieclip.h:95](../include/flashvm/movieclip.h#L95) |
| `fvm_movieclip_replace_image` | function | [include/flashvm/movieclip.h:125](../include/flashvm/movieclip.h#L125) |
| `fvm_movieclip_replace_resource` | function | [include/flashvm/movieclip.h:129](../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_replace_swf` | function | [include/flashvm/movieclip.h:114](../include/flashvm/movieclip.h#L114) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->

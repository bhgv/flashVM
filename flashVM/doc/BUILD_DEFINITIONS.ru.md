# Выбор сборки через DEFS.txt

Корневой `DEFS.txt` — основной файл ключей сборки. В каждой группе нужно
раскомментировать ровно один ключ, оставив `#` перед остальными. Допускается
комментарий после ключа. Это параметры компиляции, а не настройки ActionScript.
Неизвестные ключи, повторы, пустые группы и конкурирующие варианты отклоняются.

| Группа | По умолчанию | Альтернативы |
| --- | --- | --- |
| Растеризатор | GRAPHICS_AGG | GRAPHICS_CAIRO, NO_GRAPHICS |
| Целевая ОС | AUTO_PLATFORM | WINDOWS, LINUX, MAC, ANDROID |
| Вывод | HEADLESS | FB, GLES, OGL, DX, SDL1, SDL2, SDL3 — пока не реализованы |
| База данных | SQLITE | NO_SQLITE |
| Нативный ввод/TCP | OS_PORTS | NO_OS_PORTS |
| Задания отрисовки | RENDER_JOBS | NO_RENDER_JOBS |
| Исполнение | INTERPRETER | JIT — автоматическая компиляция с fallback в интерпретатор; режимы API: EXECUTION_MODES.md |
| Мультимедиа | NO_MEDIA | MEDIA — пока не реализован |

[`AUTO_PLATFORM`](../cmake/BuildDefinitions.cmake#L8) выбирает целевую ОС из toolchain CMake. Явный ключ ОС должен
совпадать с целью: замена LINUX на WINDOWS не превращает Linux-компилятор в
кросс-компилятор Windows. Android распознаётся до общего Linux-варианта.
Адаптеры FlashVM берутся из `ports/<платформа>/`.

[`HEADLESS`](../cmake/BuildDefinitions.cmake#L9) означает работу с буфером RGBA8 приложения. Окно и подключение к экрану
не создаются; вывод выполняет встраивающее приложение. Зарезервированные экранные
ключи при выборе завершают конфигурацию явной ошибкой. Наличие GL/Win32-кода
в исходниках Cairo не означает наличие экранного адаптера FlashVM.

[`GRAPHICS_CAIRO`](../cmake/BuildDefinitions.cmake#L7) собирает встроенные Cairo/Pixman и включает Cairo в API
растеризации FlashVM. [`GRAPHICS_AGG`](../cmake/BuildDefinitions.cmake#L7) выбирает встроенный AGG. [`NO_GRAPHICS`](../cmake/BuildDefinitions.cmake#L7)
не собирает ни одну графическую библиотеку; корректный вызов растеризации
возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). zlib и mini_regex включены во всех вариантах.
[`SQLITE`](../cmake/BuildDefinitions.cmake#L10) собирает amalgamation, C-адаптер и SQLite-программы; поиск или подмена
системной библиотекой отсутствует.

После выбора ключей создайте новую сборку:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure

Для независимых вариантов скопируйте файл, измените копию и задайте
`-DFVM_DEFS_FILE=/абсолютный/путь/my-defs.txt`. Абсолютный путь устраняет
неоднозначность рабочего каталога. Изменение DEFS.txt автоматически запускает
повторную конфигурацию при следующей сборке. Заголовок
`generated/flashvm_build_config.h` создаётся в каталоге сборки; выбранные ключи
также передаются как публичные макросы цели flashvm.

В исходном файле включено по одному ключу в группе. Сгенерированный заголовок
содержит AUTO_PLATFORM и ровно один разрешённый ключ ОС. Старые параметры кеша
FVM_ENABLE_AGG/FVM_ENABLE_CAIRO/FVM_ENABLE_SQLITE/FVM_ENABLE_OS_PORTS/
FVM_ENABLE_RENDER_JOBS допускаются только при совпадении с файлом. При конфликте
старый кеш нужно удалить либо создать новый каталог сборки.
FVM_BUILD_TESTS/FVM_BUILD_EXAMPLES остаются независимыми параметрами CMake;
FVM_ENABLE_RENDER дополнительно может потребовать наличие растеризатора.

На Linux без CMake: `python3 scripts/check.py` читает те же ключи и собирает те же
выбранные списки исходников. Доступны `--defs /путь/к/файлу` и `--sanitize`.
Этот скрипт проверяет только Linux; для других целей используется CMake.
`python3 scripts/check_definitions.py` проверяет ошибки выбора и повторную
конфигурацию; `--cmake /путь/к/cmake` позволяет выбрать отдельную установку.
`python3 scripts/check_vendor_sources.py` проверяет манифест встроенных исходников.

Наличие ключей не означает завершение SWF-отрисовки, полноту JIT или проверку
Windows/macOS/Android. Промежуточный premultiplied-буфер Cairo может округлять RGB
при малой альфе иначе, чем AGG; отступы и нетронутые пиксели сохраняются.
См. [подтверждающие проверки](DEPENDENCY_CHECKPOINT.ru.md).

IMAGE_CODECS (по умолчанию) / NO_IMAGE_CODECS — отдельная группа библиотек
PNG/GIF/JPEG/SVG. При включении Cairo/Pixman собираются также с AGG или
NO_GRAPHICS для экспорта SVG; выбор основного растеризатора не меняется.
Подробнее: [API и ограничения](IMAGE_CODECS.ru.md).

FONTS (по умолчанию) / NO_FONTS — FreeType, HarfBuzz, libunibreak и FriBidi,
адаптеры FreeType для выбранных AGG/Cairo, нативные API шрифтов/контуров.
Все цели external статические независимо от BUILD_SHARED_LIBS. См.
[подключение шрифтов](FONT_LIBRARIES.ru.md).

See / см. [JIT build-default contract](JIT_BUILD_DEFAULT.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `AUTO_PLATFORM` | DEFS switch | [cmake/BuildDefinitions.cmake:8](../cmake/BuildDefinitions.cmake#L8) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `GRAPHICS_AGG` | DEFS switch | [cmake/BuildDefinitions.cmake:7](../cmake/BuildDefinitions.cmake#L7) |
| `GRAPHICS_CAIRO` | DEFS switch | [cmake/BuildDefinitions.cmake:7](../cmake/BuildDefinitions.cmake#L7) |
| `HEADLESS` | DEFS switch | [cmake/BuildDefinitions.cmake:9](../cmake/BuildDefinitions.cmake#L9) |
| `NO_GRAPHICS` | DEFS switch | [cmake/BuildDefinitions.cmake:7](../cmake/BuildDefinitions.cmake#L7) |
| `SQLITE` | DEFS switch | [cmake/BuildDefinitions.cmake:10](../cmake/BuildDefinitions.cmake#L10) |
<!-- source-links:end -->

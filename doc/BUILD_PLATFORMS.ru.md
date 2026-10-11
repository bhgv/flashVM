# Офлайн-сборка и проверка платформ

CMake 3.10+ выбирает строгие C90/C++98; GCC/Clang используют pedantic-errors
и ошибку для неявных объявлений C-функций. Все нужные сторонние исходники
находятся в external/. Загрузок при конфигурации и подмены системными
Cairo/SQLite нет. Форма команд -S/-B требует CMake 3.13+; старые версии
можно запускать из каталога сборки командой `cmake ..`.

Измените корневой DEFS.txt либо задайте FVM_DEFS_FILE; см.
[ключи сборки](BUILD_DEFINITIONS.ru.md). AUTO_PLATFORM следует целевому
компилятору; явный ключ ОС должен совпадать с целью. По умолчанию включены
AGG, встроенная SQLite и HEADLESS. GRAPHICS_CAIRO выбирает Cairo/Pixman.
NO_GRAPHICS оставляет API растеризации с ответом UNSUPPORTED. Программы
проверок отключённых адаптеров исключаются.

Linux/macOS: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, затем
`cmake --build build --parallel`, `ctest --test-dir build --output-on-failure`.
Windows: выбрать компилятор/generator, выполнить конфигурацию, затем
`cmake --build build --config Release` и
`ctest --test-dir build -C Release --output-on-failure`. MSVC не имеет
отдельного режима C++98; совместимость требует реальной сборки и запуска.
Android: указать NDK android.toolchain.cmake, ANDROID_ABI и ANDROID_PLATFORM;
запускать тесты на устройстве/эмуляторе. NDK и SDK в архив не включены.

Скалярный профиль Cairo/Pixman сохраняет потоки и нативные 64-битные типы,
но не требует экранных/шрифтовых пакетов и ассемблерных CPU-флагов. Собственный
VFS SQLite остаётся в external/. Адаптеры ОС FlashVM расположены в ports/.
Нужны toolchain, системные заголовки и библиотеки потоков/математики ОС.

CMake обнаруживает тесты/примеры, сохраняет assert в Release и задаёт таймаут
30 секунд. После добавления исходника теста повторите конфигурацию.
`scripts/check.py` собирает Linux strict/sanitizer по тому же DEFS.txt и спискам
библиотек. Ограничения sanitizer указаны в
[проверках коммита](DEPENDENCY_CHECKPOINT.ru.md).

Проверено выполнение только на Linux x86_64. Платформенные исходники и настройки
не доказывают готовность Windows/macOS/Android. Ключи FB/GLES/OGL/DX/SDL1/SDL2/SDL3,
JIT и MEDIA зарезервированы; их выбор завершает конфигурацию ошибкой.

FONTS добавляет встроенные статические FreeType 2.14.3, HarfBuzz 1.8.8,
libunibreak 6.1, FriBidi 1.0.16 и адаптеры FreeType для AGG/Cairo. Fontconfig
и поиск системных шрифтов отключены. [Профиль и ограничения](FONT_LIBRARIES.ru.md).

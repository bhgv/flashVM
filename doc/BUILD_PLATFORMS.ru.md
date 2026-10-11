# Автономная сборка и проверки платформ

CMake 3.10+ задаёт строгие C90/C++98; GCC/Clang используют pedantic-errors.
zlib и AGG собираются из включённых исходников. При конфигурации скачиваний нет.

Linux/macOS: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`, затем
`cmake --build build --parallel`, `ctest --test-dir build --output-on-failure`.
Windows: выбрать компилятор/генератор, выполнить конфигурацию, затем
`cmake --build build --config Release`,
`ctest --test-dir build -C Release --output-on-failure`. У MSVC нет отдельного
режима C++98; совместимость требует настоящего запуска этого компилятора.
Android: указать NDK android.toolchain.cmake, ANDROID_ABI и ANDROID_PLATFORM.
Тесты запускаются на устройстве/эмуляторе, не на машине сборки. SDK/NDK не входят
в архив.

Опции: FVM_BUILD_TESTS, FVM_BUILD_EXAMPLES, FVM_ENABLE_AGG,
FVM_ENABLE_OS_PORTS, FVM_ENABLE_RENDER_JOBS, FVM_ENABLE_SQLITE. Примеры/тесты
отключённых адаптеров исключаются. При отключении тестов и примеров собираются
только библиотеки. Без AGG API растеризации возвращает UNSUPPORTED. SQLite
остаётся опциональной системной зависимостью. JIT и media пока отсутствуют;
запрос их включения вызывает ошибку конфигурации.

Тесты/примеры регистрируются общим CMake-helper, с активными assert в Release
и таймаутом 30 секунд. После добавления тестов повторить конфигурацию.
`scripts/check.py` проверяет Linux strict/sanitizer; SQLite исключён, zlib/AGG
собираются из исходников. Ограничения sanitizer описаны в VALIDATION.

Пока подтверждён только запуск Linux. Выбор адаптеров не подтверждает готовность
Windows/macOS/Android. Старые отчёты исторические; отчёты этой фазы:
`doc/validation/phase2-*`. Инструменты сборки не входят в ZIP с исходниками.

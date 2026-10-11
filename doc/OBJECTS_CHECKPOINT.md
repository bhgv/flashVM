# v0.13 validation checkpoint / Проверка этапа v0.13

Scope and remaining acceptance gates: [EN](STANDARD_OBJECTS.md),
[RU](STANDARD_OBJECTS.ru.md). **Simple types and MovieClip are not complete.**

| Linux profile | Passed tests/examples/process drivers |
| --- | ---: |
| Headless, bundled SQLite, OS ports | 162 |
| Bundled static AGG, fonts/codecs/SQLite, OS ports | 172 |
| Bundled static Cairo, fonts/codecs/SQLite, OS ports | 172 |
| No graphics/SQLite/OS ports/render jobs/fonts/image codecs | 153 |
| Strict standalone ASan/UBSan, headless, no SQLite, OS ports | 159 |

GCC/G++ 13.3, CMake 3.31.10. Project targets compile in C90/C++98 with
pedantic-errors; new public APIs are exercised by C tests. The VM implementations
use project-owned containers, not STL containers. External static dependencies
retain their earlier pinned versions.

The first Cairo pass had execution-permission failures for date_timezone and its
process driver. Restoring the executable bit and rerunning both failed entries
passed. This was a launch failure, not a calendar assertion failure. Technical
logs and copied profiles are excluded from Git and are delivered separately.

Sanitizer reproduction uses scripts/check.py --sanitize --no-asan-stack and a
headless/no-SQLite OS-port DEFS file. All ASan stack checks are explicitly disabled
because libasan cannot correctly discover/unpoison exception stacks in this
managed environment. Heap/global ASan and UBSan remain active. LeakSanitizer is
disabled because /proc access is unavailable; container_exceptions.cpp uses UBSan
only. Thus this checkpoint is **not a full stack/leak sanitizer certification**.
No Windows/macOS/Android runtime verification is claimed.

Профили Linux проверены с GCC/G++ 13.3 в строгих C90/C++98. Новые C API проверяют
C-тесты; контейнеры VM собственные. Версии статических внешних библиотек прежние.
В первом проходе Cairo две проверки не запускались из-за прав исполнения; после
восстановления executable bit обе повторные проверки прошли. Логи и копии
профилей в Git не включены.

Санитайзеры запускались с --no-asan-stack: стековые ASan-проверки отключены явно
из-за ограничений обработки стека исключений libasan в этой среде. Heap/global
ASan и UBSan включены; LeakSanitizer отключён из-за недоступного /proc, тест
container_exceptions.cpp работает только под UBSan. Полная проверка стека/утечек
и реальная работа на Windows/macOS/Android этим этапом не подтверждаются.

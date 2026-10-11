# Validation artifacts / Результаты проверок

Build/test logs, generated JSON summaries, source hashes and copied configuration
files are auxiliary artifacts, not source code. Since v0.6 they are excluded
from the tracked tree. Keep them outside the repository or in ignored build
output. Actual tests, reusable build profiles and concise bilingual checkpoint
reports remain tracked. Historical commits are preserved; some older reports
refer to log files which have now been removed from the current tree.

Логи сборок/тестов, автоматически созданные JSON-сводки, хеши исходников и копии
конфигураций — вспомогательные результаты. С v0.6 они не входят в отслеживаемое
дерево. Храните их вне репозитория или в игнорируемых каталогах сборки. Тесты,
повторно используемые конфигурации и краткие двуязычные отчёты остаются в Git.
История не переписана; ссылки старых отчётов на удалённые логи являются
историческими. Независимые исходники воспроизведения ошибок санитайзеров
перенесены в `scripts/probes/`.

Reproduce / Повторить: configure and build with CMake, then run CTest;
`python3 scripts/check.py --sanitize --limited-asan-stack` runs the compiler and
sanitizer suite with the explicitly documented exception-stack restriction.

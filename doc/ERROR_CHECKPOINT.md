# v0.9 Error checkpoint / Проверенный этап Error

This development snapshot adds AS2-style Error constructor/prototype metadata,
native message conversion, GC roots, graph transfer and a native-class instanceof
bridge with AVM1 opcode dispatch. Native resource exceptions now support Error
stringification; mutex fault context includes resource kind and origin. English
and Russian API documentation and an ActionScript illustration are included.

В этой версии добавлены метаданные Error по модели AS2, преобразование сообщения,
корни GC, передача графов и instanceof нативных классов из C API и AVM1.
Ресурсные исключения поддерживают преобразование в строку; контекст сбоя мутекса
содержит вид ресурса и место создания. Добавлены двуязычная документация и пример.

## Verification / Проверки

Linux x86_64, GCC/G++ 13.3.0, CMake/CTest 3.31.10, strict C89/C++98:

| Configuration / Конфигурация | Passed / Пройдено |
| --- | ---: |
| Headless, SQLite, OS ports; no graphics/fonts/codecs | 142/142 |
| AGG, SQLite, fonts, image codecs, OS ports | 152/152 |
| Cairo, SQLite, fonts, image codecs, OS ports | 152/152 |
| No OS ports, graphics, SQLite, fonts or codecs | 134/134 |
| Strict compiler runner, no SQLite, ASan heap/global + UBSan | 139/139 |

Coverage includes constructor defaults/value preservation, prototype mutation,
throwing native message getters through AVM1 catch/finally, instanceof identity,
GC after removing the global binding, cyclic queue transfer, separate file
writer/reader processes without destination standard-library installation,
resource exception stringification and injected allocation failures during
installation, construction and resource exception materialization. Failed
allocation must return an error rather than leak a C++ exception through the C
interface; materialization can be retried after collection.

Проверены значения по умолчанию и сохранение аргумента, изменение прототипа,
исключение getter через AVM1 catch/finally, идентичность instanceof, GC после
удаления глобального имени, циклы при передаче в очередь, независимые процессы
записи/чтения файла без стандартной библиотеки получателя, ресурсные исключения
и отказы выделения памяти. Неудачное создание исключения допускает повтор после GC.

### Sanitizer limits / Ограничения санитайзеров

The first ASan run with stack instrumentation stopped after its runtime explicitly
reported that stack discovery/unpoisoning during C++ exception unwinding failed
in this managed environment. The final run used `--no-asan-stack`: ASan heap and
global checks and UBSan remained enabled, including the new allocation test.
All ASan stack checks and LeakSanitizer were disabled. The historical
container_exceptions test still uses UBSan only. SQLite was disabled for this
sanitizer profile; it was tested in the three ordinary configurations above.
The failed initial sanitizer output is preserved in the separate validation ZIP.
A full ASan stack/LSan/SQLite run on an unrestricted host remains necessary.

Первый запуск ASan со стеком остановился после сообщения runtime о невозможности
определить и очистить стек при раскрутке C++-исключения. Итоговый запуск использует
`--no-asan-stack`: проверки кучи и глобальной памяти ASan и UBSan включены, в том
числе в новом тесте отказов памяти. Проверки стека ASan и LeakSanitizer выключены;
старый container_exceptions проверяется только UBSan. SQLite в этой конфигурации
выключен, но проверен в обычных сборках. Вывод первого неудачного запуска сохранён
отдельно. Полная проверка на хосте без этих ограничений ещё требуется.

## Delivery and remaining scope / Выдача и оставшаяся работа

Raw logs, copied profiles and generated test results are not committed. They are
provided in FlashVM-validation-v0.9.zip, separately from the authoritative source
archive with complete Git history. The explicitly requested mirror update was
verified at v0.8 commit 3721f406b5c05ac402db73629ec0614a8bf35f11 before v0.9 work.
The v0.9 commit is internal; another mirror push requires a direct command.

Логи, копии профилей и сгенерированные результаты не включены в новый коммит.
Они выдаются отдельным архивом FlashVM-validation-v0.9.zip. Основной архив содержит
исходники и полную историю Git. Зеркало по прямой команде обновлено и проверено на
v0.8 до начала этого этапа; v0.9 не отправляется автоматически.

This is not completion of all Flash objects, SWF 8, JIT or all target platforms.
See STANDARD_ERROR.md, STANDARD_ERROR.ru.md and SWF8_ROADMAP.md for concrete limits.
Полная реализация всех объектов Flash, SWF 8, JIT и целевых платформ не завершена.

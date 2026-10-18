# Transform constants / Константы Transform

v0.331 names the native Transform property schema, affine snapshot component
indices and historical callback identities in src/standard/transform_constants.h.
The serialized identities remain constructor 1400, MovieClip getter 1401,
property getters 1410–1414 and setters 1420–1421. Property attribute expressions
use the public FVM_PROPERTY flags. This is a readability change, with no new
Flash compatibility claim or graph format revision.

В v0.331 схема свойств Transform, индексы компонентов матрицы и постоянные ID
callbacks вынесены в src/standard/transform_constants.h с пояснениями. Значения
ID и порядок свойств сохранены; флаги свойств используют публичные макросы
FVM_PROPERTY. Это улучшение читаемости без изменения формата переноса объектов
и без объявления новых возможностей Flash или завершения стадий роудмапа.

transform_callback_ids independently checks the historical raw IDs, uniqueness
and rejection of gaps/boundaries. Existing standard_transform and
transform_allocation exercise snapshots, graph transport and allocation errors.

Validation: fresh Linux GCC 13.3 Release build using the default AGG/DEFS profile;
649/649 CTest tests passed (643 initially, six executable-permission launch failures
passed after restoring execution rights). This run includes the callback-ID,
Transform/ColorTransform allocation/transport and drag regressions. No Cairo,
sanitizer or other-platform rerun is claimed for this constant-only change.

Проверка: свежая Linux Release-сборка GCC 13.3 с исходным профилем AGG/DEFS,
649/649 тестов. Шесть запусков потребовали восстановления прав исполнения;
после этого все прошли. Для этого изменения отдельный прогон Cairo, санитайзера
и других платформ не заявляется.

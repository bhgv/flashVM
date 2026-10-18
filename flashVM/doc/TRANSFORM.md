# flash.geom.Transform и MovieClip.transform

Transform хранит управляемую ссылку на MovieClip. MovieClip.transform возвращает
новый Transform, связанный с исходным экземпляром. Конструктор принимает ровно
один MovieClip; другие аргументы дают несвязанный объект с undefined-свойствами.
Статические внутренние узлы shape не принимаются как MovieClip.

matrix и colorTransform возвращают независимые Matrix/ColorTransform. Их запись
копирует числовые значения в сцену; последующие изменения исходного или
возвращённого объекта не изменяют сцену. Для применения изменённого снимка
его нужно присвоить обратно. concatenatedMatrix/concatenatedColorTransform
учитывают родителей и доступны только для чтения. pixelBounds возвращает
Rectangle с мировыми векторными границами, округлёнными наружу до пикселей.
Границы используют существующую модель path/stroke; bitmap, текст, фильтры
и маски ещё не покрыты полностью.

Запись цвета атомарно меняет скопированную RGBA-запись и _alpha. Alpha-множитель
сохраняется как коэффициент: ограничение 0..255 происходит после применения
цветового смещения при отрисовке. _alpha синхронизирован с alphaMultiplier.
Нефинитные матрицы/применяемые цвета отклоняются; точное fixed-point поведение
исторического Player и необычные coercion-сценарии ещё требуют сверки.

Ссылка Transform/клип передаётся обычным сериализатором. При передаче их в одном
графе ссылка остаётся общей в принимающем контексте; она не ссылается обратно
на исходный контекст. Приватная запись содержит только clip: MovieClip либо
undefined. Повреждённое имя поля отклоняется. Getter-фабрики сохраняют свои
классы через корни GC и работают без установки стандартной библиотеки у получателя.

standard_transform.cpp проверяет локальные/мировые матрицы и RGBA, копирование,
readonly, мировые границы, удержание сцены через Transform, queue/file/loopback
TCP с сохранением связи, malformed state и пиксель alpha с положительным
множителем >1 и отрицательным смещением в графических сборках.
transform_allocation.cpp перебирает отказы записи цвета и всех getter-фабрик;
проверяет неизменность сцены при неудачной записи. Общий installer injection
покрывает порядок установки класса и MovieClip.transform.

Открыто: полная модель удаления/повторного создания клипа и ссылок по путям,
bitmap/текстовые/фильтровые pixelBounds, scrollRect, маски, точная числовая
квантизация Player, все getter/coercion/прототипные overrides и реальные
удалённые процессы. Этапы 3 и 4 не закрываются этой контрольной точкой.

## English

Transform retains a managed MovieClip reference; MovieClip.transform creates a
new associated wrapper. Local matrix/color properties return independent typed
snapshots and copy assignments into scene state. Composed properties are readonly
and include ancestor transforms. pixelBounds rounds world vector/stroke bounds
outwards; bitmap/text/filter/mask bounds remain incomplete.

Color publication atomically updates copied RGBA state and _alpha. Rendering
clamps final alpha after applying its multiplier and offset. Nonfinite applied
values are rejected; full historical fixed-point/coercion conformance is pending.

Graph transport preserves the common clip reference within a message, including
queue/file/loopback TCP. Restored factory callbacks retain geometry classes and
work without installing standard globals. Validation rejects malformed private
reference records. Tests cover scene composition, independent snapshots, GC,
transport identity, allocation failure and the graphical alpha regression.

Removed/recreated clip reference semantics, scrollRect, masks, complete pixelBounds,
Player numeric quantization, unusual overrides and remote-process coverage remain
open. No roadmap stage or complete Flash compatibility is certified.

## Проверки v0.36 / Validation

Linux: headless 194, AGG/Cairo по 204, portable 185. Первый полный запуск выявил
устаревшую границу 2048 в error_allocation.cpp: тест теперь измеряет успешную
установку и перебирает все её точки выделения, включая новую namespace flash.
Исправленный тест прошёл во всех конфигурациях. Три headless, один AGG и два
Cairo теста не запустились из-за execute permissions; повтор после восстановления
прав прошёл. Strict C89/C++98 с ASan/UBSan: 191 тест/пример; первый sanitizer
запуск остановился на старом ограничении теста, полный повтор с исправлением
прошёл. ASan stack/LeakSanitizer отключены из-за среды, heap/global ASan и UBSan
включены; container_exceptions проверен только UBSan. Остальные ОС не проверены.

All listed Linux cases passed, with explicit allocation-test correction and
permission-related retries. The complete corrected strict-language/sanitizer run
passed 191 programs under the instrumentation limitations above. No complete
Flash Player or other-platform acceptance is claimed; logs remain outside Git.

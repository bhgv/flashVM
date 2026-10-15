# План завершения FlashVM / FlashVM completion roadmap

Принят пользователем 12 октября 2026 года. Основной ориентир дальнейшей
разработки. Исходная контрольная точка: v0.14. Добавление этого документа в
v0.15 не означает появления новых возможностей VM.

Цель: полная реализация Flash SWF ≤ 8 и соответствующих AS1/AS2 API,
запрошенные расширения, JIT и поддержка Windows/Linux/macOS/Android.
Требования: C89/C++98, без STL-контейнеров; платформенные адаптеры в ports/;
внешние библиотеки в external/, статическое подключение; читаемый код и
комментарии, тесты, документация на русском и английском.

## Этапы

| № | Этап | Оставшаяся работа |
| --- | --- | --- |
| 1 | Простые типы и стандартные объекты | Полноценные UTF-16 строки, встроенный NUL, одиночные суррогаты, Unicode-регистр; оставшаяся семантика Object/Array, наследуемые индексы, заимствованные методы, registerClass. |
| 2 | Интерпретатор AVM1 | DefineFunction2, регистры и preloads, связанность arguments с параметрами, области видимости, интерфейсы, исключения и точные правила преобразований для разных версий SWF. |
| 3 | MovieClip и сцена SWF | Связать модель MovieClip с ресурсами и display list SWF; attach/duplicate, глубины, кадры, маски, размеры, shape hit testing, drag/input, загрузка клипов и создание текстовых полей. |
| 4 | Графика, текст и мультимедиа | Градиенты, bitmap-заливки, фильтры и blend modes; полное поведение TextField/TextFormat, встроенные и системные шрифты; звук, потоковое аудио и видео соответствующих версий Flash. |
| 5 | Остальные стандартные API | Полный перечень объектов и методов AS1/AS2 и реализация недостающих: загрузчики, XML, сетевые объекты, SharedObject, события, ввод и системные службы. |
| 6 | Расширения FlashVM | Согласованная модель потоков, каналов, мутексов, метатаблиц и сериализации всех объектов, включая встроенные; перенос ресурсных аннотаций; сетевые обрывы, устаревшие ссылки и распределённые блокировки; API C-расширений по образцу Lua. |
| 7 | GC и отладчик | Подтвердить полноту корней AVM1, нативных функций и новых подсистем, финализацию и циклы при конкуренции; дедлоки с привязкой к исходникам, перехват и заморозка посылок каналов для анализа. |
| 8 | JIT | Генерация кода ARM, ARM64, x86, x64, RISC-V и RISC-V64; согласование с GC, исключениями, отладчиком и интерпретатором; проверка эквивалентности исполнения. |
| 9 | Переносимость и выпуск | Реальные сборки и запуски на Windows, Linux, macOS и Android; статические зависимости, выбранные графические адаптеры, документация RU/EN и воспроизводимые архивы с полной историей Git. |

Все этапы остаются открытыми; реализованные части не означают завершения этапа.
Конкретные подтверждённые возможности исходной точки и ограничения описаны в
[STANDARD_OBJECTS.ru.md](STANDARD_OBJECTS.ru.md), [PROPERTY_WATCH.md](PROPERTY_WATCH.md)
и [OBJECTS_CHECKPOINT.md](OBJECTS_CHECKPOINT.md).

## Порядок работы и контроль выполнения

Ближайший порядок: этап 1 → этап 2 → этап 3 → этапы 4 и 5.
Этапы 6 и 7 сопровождают разработку каждого объекта и подсистемы; их проверки
нельзя откладывать до конца. Этап 8 требует устойчивой семантики интерпретатора.
Платформенные проверки этапа 9 наращиваются по мере реализации, затем проходят
общую приёмку.

Перед продолжением разработки читать этот план и выбирать следующий конкретный
пункт. Вести реестр совместимости SWF/AS с отметками «отсутствует», «реализовано»,
«проверено», «завершено». Для проверенных пунктов указывать реальные тесты,
конфигурации и оставшиеся ограничения. Пересматривать реестр при изменениях.
Закрывать этап только после подтверждающих тестов и обновления RU/EN документации.
Не оценивать процент готовности или срок без полного реестра и основания.

Каждый коммит: повысить версию, использовать английское сообщение с префиксом
v<версия>, провести необходимые проверки. Внутренний Git основной. По действующей
команде пользователя обновлять зеркало после каждого коммита без force push,
с HTTP-буфером 67108864; после fetch проверять серверный коммит. Это правило
действует до нового указания пользователя.

## Итоговая приёмка

- Реальные SWF версий до 8 включительно и сравнение поведения с эталоном;
  полный реестр объектов, методов, Action-кодов и поддерживаемых SWF-тегов.
- Сериализация и десериализация всех заявленных типов через очередь, файл и сеть;
  циклы, общие ссылки, ресурсы и метатаблицы; исключения с кодом, текстом и контекстом.
- Повреждённые входные данные, отказы выделения памяти, финализация, корни GC;
  проверки памяти с явно указанными ограничениями инструментов.
- Длительные многопоточные и сетевые испытания, обрывы соединений и дедлоки;
  работоспособность средств отладки.
- Совпадение результатов интерпретатора и каждого заявленного JIT backend.
- Сборка и запуск на каждой целевой платформе и проверка поддерживаемых адаптеров.
- Документация и примеры RU/EN; исходники с полной историей Git в архиве с версией.
  Технические результаты выдаются отдельным архивом и не коммитятся.

Уточнение выдачи от пользователя 15 октября 2026 года: продолжать в существующей
рабочей копии и обновлять только зеркало. Не создавать и не выдавать архивы
исходников/локального Git до новой явной просьбы. Это заменяет прежнюю выдачу
архивов; проверки воспроизводимости и сохранение истории в зеркале остаются.

Количество прошедших тестов само по себе не доказывает полноту совместимости.
Особенно велики оставшиеся объёмы MovieClip/SWF, мультимедиа и JIT.

## English reference

This is the user-approved completion roadmap, adopted on 12 October 2026.
The implementation baseline is v0.14; v0.15 records the plan without adding VM
features. All nine stages remain open:

1. Complete primitive types and Object/Array semantics, including full UTF-16.
2. Complete AVM1 execution, Function2, registers, arguments and version semantics.
3. Integrate MovieClip with the full SWF display list and resource model.
4. Complete graphics, text, audio and video for the supported Flash versions.
5. Inventory and complete the remaining standard AS1/AS2 objects and APIs.
6. Complete threads, channels, mutexes, metatables, graph transport and Lua-style
   C extensions, including remote failures and shared resource identity.
7. Verify GC roots/cycles/finalization throughout, and complete debugger support
   for source locations, deadlocks and intercepted/frozen channel payloads.
8. Implement and verify ARM/ARM64/x86/x64/RISC-V/RISC-V64 JIT backends.
9. Verify real Windows/Linux/macOS/Android builds and deliver documented releases.

Work through stages 1, 2 and 3 first, then 4 and 5. Stages 6 and 7 accompany all
implementation work. JIT depends on stable interpreter semantics. Platform
verification grows throughout and is part of final acceptance.

Read this roadmap before resuming development. Maintain a detailed compatibility
inventory with absent/implemented/tested/complete states, test evidence, profiles
and limitations. Do not close a stage without acceptance tests and RU/EN docs.
Do not claim a percentage or deadline without an evidence-based inventory.
Final acceptance requires real SWFs, reference comparisons, malformed-input and
allocation-failure tests, graph transport tests, concurrency/network endurance,
GC/memory checks, JIT equivalence and real runs on every target platform.
Keep C89/C++98 and project-owned containers; static libraries belong in external/,
platform adapters in ports/. Internal Git is authoritative. Increment the version
for each commit and use an English v<version> subject. The current standing user
instruction authorizes a verified mirror update after every commit until revoked.
Ship versioned source archives with full Git history and separate validation
archives. Never commit generated validation logs.

Delivery update, 15 October 2026: the user's current instruction is to continue
in the existing checkout and update the verified mirror only. Do not create or
export source/local-Git archives unless explicitly requested again. This replaces
the earlier archive-delivery requirement, while build reproducibility and full
history preservation in the mirror remain acceptance requirements.

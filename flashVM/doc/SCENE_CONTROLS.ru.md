# Снимки управляющих записей сцены — v0.634

[English](SCENE_CONTROLS.md). При [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) размещение символа SWF
может вызвать зарегистрированный конструктор. Раньше он мог заменить будущий
элемент списка, изменить его скалярные поля, очистить список или добавить новое
размещение прямо в текущий обход. Теперь значимые поля управляющих записей кадра
копируются до применения первого размещения.

Список кадра — плотная последовательность индексов от нуля, независимо от порядка
вставки, не более [`FVM_MOVIECLIP_MAX_SCENE_CONTROLS`](../src/standard/movieclip_constants.h#L25) элементов. Пропуск, null,
значение не Object или ссылка на чужой контекст дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45)
до применения любой записи этого локального списка. Семантическая ошибка внутри
структурно допустимой записи может возникнуть после уже применённых записей.
Лимит независим от числа блоков действий кадра и инициализаторов.

Копируются op/depth/token/character/flags/background и необязательные symbol,
matrix, color, ratio, name, clip_actions. Неизвестные поля игнорируются. Замена
элемента списка или скалярного поля захваченной записи влияет только на дальнейшую
оценку сцены. Добавления откладываются до повторной оценки этого кадра; обычный
следующий tick не обязан повторять его controls. Изменения исходного списка
сохраняются после возврата. Ресурсные объекты остаются общими: содержимое symbol/
matrix/color/clip_actions этот поверхностный снимок не замораживает. Нового
публичного API редактирования метаданных и неизменяемого снимка всего ролика нет.

Восстановление при seek/loop использует тот же локальный снимок каждого посещённого
кадра и не исполняет пропущенные frame actions. Ранее выполненные размещения,
эффекты конструкторов и выбор фона сохраняются при последующей ошибке.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) при копировании не применяет этот список, но не откатывает
ранние кадры и не меняет существующую политику повторного запуска после ошибки.

[GC:] Действующий native barrier сцены/tick удерживает значения и строки после
удаления их из источника. Сборка внутри конструктора по-прежнему отвергается;
после вызова хоста разрешена. Native userdata заимствован и должен жить до конца
вызова. C ABI и формат сериализации не меняются.

Проверки: `scene_controls_snapshot` (SWF6–8, интерпретатор и настоящий native JIT,
четыре изменения и перемешанный порядок вставки), `scene_controls_invalid`
(пропуски/типы/контексты/лимит), `scene_controls_allocation` (сбой каждого выделения,
освобождение и GC). Существующие scene/frame/initializer tests также применимы.
[C89 пример tick](../examples/embedding/movieclip/README.ru.md) показывает вызовы
в потоке владельца и сохранение эффектов; private mutation fixtures являются
тестами, а не примером публичного API. [Команды сборки](SCENE_CONTROLS.md).

Все девять стадий остаются открытыми. Эти проверки FlashVM не подтверждают lifecycle
оригинального Player или запуск на чужих ОС и физических устройствах.

## Бюджет сцены (v0.635)

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) охватывает подготовку сцены и callbacks, наследуя общий
бюджет AVM1/tick вместо сброса на каждом control или конструкторе. Прямой переход
расходует по единице за копирование записи и перед её применением. Сбой capture
не применяет ни одной записи локального списка. Отсутствующий/пустой список прямого
перехода не добавляет работы, сохраняя стоимость обычного пустого tick.

Восстановление расходует единицу за каждый посещённый кадр, включая пропуски,
по единице за копирование и композицию control, за копирование и проверку каждого
старого child и перед применением каждого итогового размещения/обновления.
Для F кадров, K controls, M старых children и D итоговых элементов стоимость равна
F+2K+2M+D без вложенных конструкторов и прочей работы исполнения. Старый список
ограничен [`FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN`](../src/standard/movieclip_constants.h#L32), копирование предварительно оплачено.
Сортировка глубин в эту версию не входит. Протокольные флаги, цвета и идентификаторы
именованы в [scene_constants.h](../src/standard/scene_constants.h) отдельно от лимитов обхода.

Исчерпание лимита даёт [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) без превышения счётчика; насыщенный унаследованный
счётчик не обнуляется и не переполняется. Нулевой числовой лимит сохраняет проверку
кооперативной остановки worker. Время native callback, все map operations, копии
ресурсов и размеры внутренних allocations размещения отдельно не измеряются и
не оплачиваются. Это единицы логической работы, а не ограничение реального времени.

Ранние размещения, выбранный фон, эффекты конструкторов и внешнее состояние кадра
сохраняются при последующей ошибке. Маркер завершённого кадра сцены публикуется
только при успехе; повторный вызов может повторно оценивать уже применённые controls.
Атомарного отката всей сцены или нового lifecycle scheduler нет. Проверка
`scene_controls_budget` охватывает точные границы forward/reconstruction/старых
children/пропусков/вложенности/насыщения. Два настоящих OS workers в
`scene_controls_worker` останавливаются до обхода и между native-only controls.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN` | macro / define | [src/standard/movieclip_constants.h:32](../src/standard/movieclip_constants.h#L32) |
| `FVM_MOVIECLIP_MAX_SCENE_CONTROLS` | macro / define | [src/standard/movieclip_constants.h:25](../src/standard/movieclip_constants.h#L25) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->

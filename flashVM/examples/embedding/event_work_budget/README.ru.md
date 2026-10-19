# Ограниченная доставка нативного события

Исполняемый пример C89 для хоста без окна: runtime получает бюджет в пять единиц,
регистрирует три Mouse-наблюдателя и доставляет одно событие указателя. Не нужны
клавиатура, экран, устройство, SWF или стандартное пространство скриптов.
[English и таблица API](README.md).

Сборка и запуск из компонента FlashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_event_work_budget_main
./build/flashvm_examples_embedding_event_work_budget_main
```

Результат:

```text
callbacks=1 pointer=23,45 budget exhausted after publication
```

Независимые регистрации стоят одну, три и пять единиц. Событие тратит три на
укоренённый снимок, одну на публикацию и одну на первый обработчик. Следующее
списание завершается ошибкой до второго. Пример ожидает [`FVM_ERROR`](../../../include/flashvm/flashvm.h#L43), проверяет
один завершённый callback и сохранённую позицию; автоматического повтора нет.

Назначение функций, владение и блокировки перечислены в
[индивидуальной таблице API](README.md#api). Временный pin удерживает объект при
построении; после успешного добавления его удерживает контекст. Callback заимствует
счётчик стека до уничтожения контекста, не собирает память и не блокируется.
При любой ошибке сначала уничтожается контекст, затем runtime. [main.c](main.c)
объясняет каждую непустую строку.

[Учёт и ошибки](../../../doc/INPUT_EVENT_BUDGET.ru.md),
[Mouse](../../../doc/MOUSE.ru.md), [GC](../../../doc/GC_NATIVE_ROOTS.ru.md).
Это единицы работы, не время. Аппаратная/межплатформенная приёмка, полная
Flash-семантика и прерывание произвольных блокирующих callbacks не заявляются.
Остановка worker проверяется отдельными тестами.

Именованные пределы примера: [tutorial_limits.h](tutorial_limits.h).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../../../include/flashvm/flashvm.h#L43) |
<!-- source-links:end -->

> Historical checkpoint. Актуальные изменения v0.13: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

# AVM1_FUNCTIONS — русская документация

Именованные функции AVM1 поддерживаются только без параметров. `DefineFunction` сохраняет тело, `CallFunction` выполняет его, `Return` возвращает значение. Максимальная глубина вложенных вызовов — 64. Локальные области видимости, `DefineFunction2`, `CallMethod` и полноценная рекурсия пока отсутствуют.

Подробные сигнатуры и ограничения см. в `AVM1_FUNCTIONS.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.

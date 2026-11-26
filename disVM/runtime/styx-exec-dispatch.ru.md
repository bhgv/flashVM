# Node.dispatch

[English](styx-exec-dispatch.md) · [Руководство](STYX-EXEC-PUBLISH.ru.md) · [Интерфейс](../module/styxexec.m#L39) · [Реализация](../limbo/styxexec.b#L175)

```limbo
node.dispatch(handler: StyxexecHandler): int
```

Заимствуйте инициализированный модуль обработчика и примите один запрос с Node. Обработчик выполняется в вызывающем потоке Limbo. Исключение или слишком длинный результат вызывает отказ. Результат 1 означает подтверждённый ответ или отказ, 0 означает истёкший либо недоступный запрос. Отсутствие модуля или сбой приёма даёт исключение.

```limbo
settled := node.dispatch(handler);
```

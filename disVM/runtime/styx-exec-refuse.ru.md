# Node.refuse

[English](styx-exec-refuse.md) · [Руководство](STYX-EXEC-PUBLISH.ru.md) · [Интерфейс](../module/styxexec.m#L36) · [Реализация](../limbo/styxexec.b#L168)

```limbo
node.refuse(request: ref Request): int
```

Отклоните Request с этого Node без байтов результата. Вернётся 1 при подтверждении либо 0 для истёкшего, чужого, закрытого или уже обслуженного запроса. Клиент получает ошибку вызова. Для успешного пустого ответа используйте reply.

```limbo
refused := node.refuse(item);
```

# Styxexec.publish

[English](styx-exec-publish.md) · [Руководство](STYX-EXEC-PUBLISH.ru.md) · [Интерфейс](../module/styxexec.m#L47) · [Реализация](../limbo/styxexec.b#L107)

```limbo
publish(path: string): ref Node
```

Вызовите после init. Передайте канонический абсолютный путь UTF-8 до 1023 байт с начальной косой чертой. Родительский каталог должен существовать и разрешать запись и проход. Служебное или занятое имя даёт исключение. При успехе возвращается собственный Node в парном пространстве имён.

```limbo
node := exec->publish("/tools/apply");
```

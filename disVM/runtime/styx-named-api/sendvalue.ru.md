# Port.sendvalue

[English](sendvalue.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxnamed.m#L41) · [Реализация](../../limbo/styxnamed.b#L273)

```limbo
Port.sendvalue(message: ref Message)
```

Отправляет одно типизированное сообщение по открытому каналу. Data кодируется до записи. Shared заимствует живую аренду Cell до приёма. Intrefdata заимствует проверенный прежний дескриптор. Вызов возвращается после встречи. Некорректные данные или сбой ввода-вывода дают исключение. Вызовы на канале последовательны.

```limbo
port.sendvalue(ref Message.Data(codec->text("ready")));
```

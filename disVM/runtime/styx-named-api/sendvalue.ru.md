# Port.sendvalue

[English](sendvalue.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxnamed.m) · [Реализация](../../limbo/styxnamed.b)

```limbo
Port.sendvalue(message: ref Message)
```

Отправляет одно типизированное сообщение по открытому каналу. Data кодируется до записи. Shared заимствует живую аренду Cell до приёма. Intrefdata заимствует проверенный прежний дескриптор. Modref пересылает [ссылку на переменную модуля](../STYX-MODULE-REFS.ru.md) с исходной областью и режимом доступа. Держите ссылку открытой на время вызова. Вызов возвращается после встречи. Некорректные данные или сбой ввода-вывода дают исключение. Вызовы на канале последовательны.

```limbo
port.sendvalue(ref Message.Data(codec->text("ready")));
```

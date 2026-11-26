# Port.recvvalue

[English](recvvalue.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxnamed.m#L37) · [Реализация](../../limbo/styxnamed.b#L254)

```limbo
Port.recvvalue(): ref Message
```

Ожидает следующее сообщение. Data содержит значение во владении вызывающего. Shared содержит fid ячейки, открытый до возврата. Закройте аренду после использования. Intrefdata сохраняет байты дескриптора для явно смонтированного владельца. Неверный кадр или сбой ввода-вывода дают исключение. Вызовы последовательны.

```limbo
message := port.recvvalue();
```

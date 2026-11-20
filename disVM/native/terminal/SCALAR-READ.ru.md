# styxscalarread

[English](SCALAR-READ.md) · [API](scalar_record.h) · [Test](tests/scalar/record.c)

## Прочитать скалярную S-запись

`scalar_record.h` · `styx_terminal`

Передайте ожидаемый ненулевой 32-битный тег, байты записи и их длину,
а также указатели для вида и значения. Успех возвращает 1. Ошибочные
параметры, чужой тег, неизвестный вид, неполная запись или лишние байты
дают 0. Отказ сохраняет оба выходных значения. I/O нет.

```c
styxscalarread(29, reply, length, &kind, &value);
```

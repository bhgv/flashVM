# styxscalarwrite

[English](SCALAR-WRITE.md) · [API](scalar_record.h) · [Test](tests/scalar/record.c)

## Закодировать скалярную S-запись

`scalar_record.h` · `styx_terminal`

Передайте ненулевой 32-битный тег маршрута, вид записи (вызов, значение или
ошибка), знаковое 64-битное число, изменяемый буфер, его ёмкость в байтах и
указатель длины. При успехе возвращает 1 и записывает длину. При неверных
параметрах или нехватке места возвращает 0. Буфер заимствован на время вызова.
I/O нет.

```c
styxscalarwrite(17, STYX_SCALAR_CALL, -7, request, sizeof(request), &length);
```

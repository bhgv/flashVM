# styxrpc_hybrid_big_parse

[English](hybrid_big_parse.md) · [Объявление](../include/styxrpc/hybrid.h#L76) · [Реализация](../src/hybrid.c) · [Описание таблиц](hybrid_tables.ru.md)

```c
#include <styxrpc/hybrid.h>
int styxrpc_hybrid_big_parse(const char *bytes, size_t size, int64_t *value);
```

Разберите `size` заимствованных байтов из `bytes` как точное знаковое 64-битное десятичное число. Допустимы начальные `+` и `-`, за которыми идёт хотя бы одна цифра ASCII. Пробелы, дробная часть и переполнение недопустимы. При успехе функция возвращает 1 и записывает `value`, при отказе возвращает 0 без записи. Выделение памяти и завершающий ноль не требуются.

[Исполняемая проверка C](../tests/hybrid.c)

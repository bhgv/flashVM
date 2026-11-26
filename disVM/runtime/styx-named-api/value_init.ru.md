# Styxvalue.init

[English](value_init.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxvalue.m#L67) · [Реализация](../../limbo/styxvalue.b#L19)

```limbo
init()
```

Инициализирует загруженный экземпляр Styxvalue перед работой с кодеком. Загружает модуль Math для точных преобразований binary64. Его отсутствие даёт исключение. Вызовите до передачи экземпляра между потоками Dis.

```limbo
codec->init();
```

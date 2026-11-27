# Styxmodref.init

[English](init.md) · [Объявление](../../module/styxmodref.m) · [Реализация](../../limbo/styxmodref.b) · [Руководство](../STYX-MODULE-REFS.ru.md)

`styxmodref.dis`

```limbo
init: fn();
```

Загрузите привязку Sys перед открытием ячеек. Вызовите `refs->init()` после загрузки модуля и до передачи его другим потокам Limbo. Вызов не монтирует вторую сторону. Службу ссылок предоставляет парный рантайм.

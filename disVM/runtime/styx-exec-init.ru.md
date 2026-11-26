# Styxexec.init

[English](styx-exec-init.md) · [Руководство](STYX-EXEC.ru.md) · [Интерфейс](../module/styxexec.m#L51) · [Исходник](../limbo/styxexec.b#L16)

```limbo
init: fn();
```

Подключите `styxexec.m`, загрузите установленный `styxexec.dis` и проверьте
ссылку на модуль перед вызовом. Инициализация загружает Sys. Метод
вызывается без аргументов, не возвращает значения и не открывает
сессию. Инициализируйте модуль до того, как его получат процессы,
использующие [call](styx-exec-call.ru.md).

```limbo
exec := load Styxexec Styxexec->PATH;
if (exec == nil) {
    raise "cannot load Styxexec";
}
exec->init();
```

# styxlaunchprocesspayload

[English](LAUNCH-PROCESS-PAYLOAD.md) · [API](launch_process.h) · [Настройка процесса](LAUNCH-PROCESS.ru.md) · [Пример](../../examples/vfs-run/README.ru.md)

`int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit);`

Подключите `styx_terminal`. Вызовите для инициализированного владельцем
процесса до публикации. Исполняемый файл должен поддерживать параметр
DisVM для стартовых байтов. Права и рабочий каталог задаёт существующая
настройка хоста. `limit` задаётся в байтах от 0 до 65 536. Успех возвращает 1,
неверное состояние или превышение лимита — 0. Функция заимствует состояние
и не создаёт дочерний процесс.

Например, вызовите `styxlaunchprocesspayload(&publication.process, 3)` до
публикации маршрута. Следующий `Vfsctl.runargs` передаст байты в
`Launchcontext.arguments()` до исполнения гостя. Лаунчер передаёт запечатанный Linux memfd.
В argv попадает только номер дескриптора. DisVM читает его с нулевого смещения
и закрывает перед исполнением гостя. У транспорта может быть меньший предел.
См. [`--startup-fd`](../../runtime/STARTUP-FD.ru.md).

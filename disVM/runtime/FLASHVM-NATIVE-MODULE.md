# Installed FlashVM C module example / Установленный пример C-модуля

From this example directory / Из каталога этого примера:

```sh
../../run-flashvm-native-module.sh
```

Or choose item18 in the workspace build menu. The runner executes the installed
host with the installed linked SWF. No Python, compiler, display or timeline tick
is required to run it. It propagates the program's actual exit status.
Можно выбрать пункт18 build-menu. Запускаются установленный хост и собранный SWF;
для запуска не нужны Python, компилятор, окно или шаг таймлайна. Код возврата
программы сохраняется.

`module.h` and `module.c` contain the separately compiled namespace factory,
`main.c` its C host, and `main.as` the original property-write example. The package
includes sources for reading; their rebuild instructions target a complete source
checkout, not this installed SDK. See the
[full tutorial](https://github.com/bhgv/flashVM/blob/master/flashVM/examples/embedding/native_module/README.md)
and [existing extension examples](https://github.com/bhgv/flashVM/tree/master/flashVM/examples/extending).

В `module.h/module.c` находится фабрика модуля, в `main.c` — C-хост,
в `main.as` — исходный пример записей свойств. Исходники приложены для изучения;
инструкции пересборки относятся к полному репозиторию. Модуль статически связан
с хостом: автоматического загрузчика `.so/.dll` пример не добавляет.

The host state and callback code outlive their contexts. Managed captures are
rooted explicitly. Cross-context handles are refused; VM calls stay on the owner
thread. / Состояние C и код callback живут дольше контекстов; управляемые захваты
удерживаются явно, чужие объекты отвергаются, вызовы VM идут из потока владельца.

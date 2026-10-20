# Условный переход

[English](README.md).

`if %less, ^yes, ^no` выбирает первый адрес при true и второй при false.
`jump ^done` безусловно передаёт управление. Значения из одной ветки нельзя
использовать в другой: это проверяет доминирование SSA. Параметры блоков и
phi пока не реализованы. `trace` получает одно значение и выводит строку.

```sh
build/fir1/fir1 check fir/v1/examples/control_flow/main.fir
build/fir1/fir1 opt fir/v1/examples/control_flow/main.fir build/fir1/control_flow.fir
build/fir1/fir1 object build/fir1/control_flow.fir build/fir1/control_flow.swo
build/linker/swl link build/fir1/control_flow.swf build/fir1/control_flow.swo
```

Ожидаемый trace: `yes / да`.
[CLI](../../compiler/README.ru.md), [FIR](../../doc/FORMAT.ru.md),
[tests](../../doc/TESTING.ru.md). `python fir/v1/view_docs.py`.

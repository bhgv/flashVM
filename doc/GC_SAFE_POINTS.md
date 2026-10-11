# GC safe points / Безопасные точки GC

The current collector does not scan live AVM1 evaluation stacks or all native callback temporaries. Collect only between VM execution segments; pin externally retained objects.

Текущий сборщик не сканирует активный стек вычислений AVM1 и все временные значения C-обработчиков. Запускайте сборку только между сегментами выполнения VM и закрепляйте объекты, удерживаемые нативным кодом.

`fvm_gc_step` is a threshold-driven operation; `fvm_gc_collect` forces a collection. `FVM_GC_FULL` preserves the previous mode.

# Статистика памяти из Limbo

Copyright (c) 2026 bhgv

[English](README.md) · [Контракт](../../runtime/MEMORY-STATISTICS.ru.md)

Пример запускает настоящего resident, печатает снимки до/во время/после byte allocation и
завершает гостя через killwait. Остальные owners не останавливаются.

```sh
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/memoryview.dis disVM/debug/memoryview.b
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module \
  -o build/disvm-opt/memory-statistics.dis disVM/examples/memory-statistics/main.b
build/disvm-opt/disvm --stats --no-jit build/disvm-opt/memory-statistics.dis
build/disvm-opt/disvm --stats --jit build/disvm-opt/memory-statistics.dis
```

Храните memoryview.dis рядом с программой. Успех: `PASS memory statistics example`.
Проверяйте `arena = header + live + free`. После `bytes = nil` live может ещё расти:
reclamation отложен, а сама печать и Stats тоже выделяют память. Это не requested payload
и не RSS. Capacity включает alignment и Heap headers; свободные блоки не объединяются.
Per-thread/per-type attribution ещё отсутствует.

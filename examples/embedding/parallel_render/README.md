# Parallel rendering / Параллельная отрисовка

English: `main.c` is a C89 embedding example that fills 64 RGBA pixels using
four native workers. The callback receives a pixel index and writes only to
that pixel. Compile using the CMake target `flashvm_example_parallel_render`.
This is a host-side demonstration, not Flash MovieClip rasterization.

Русский: `main.c` — пример встраивания на C89. Четыре потока заполняют
64 пикселя RGBA, каждый поток пишет в независимые ячейки буфера.
Сборка: цель CMake `flashvm_example_parallel_render`.
Это пример задач хоста, а не полноценная отрисовка Flash MovieClip.

## ActionScript 1/2 note / Примечание для ActionScript 1/2

Flash ActionScript does not control worker threads directly. An ordinary SWF
might update `_root.clip._x` in its frame script; after AVM1 advances, the host
must snapshot the display state before scheduling parallel rasterization.

ActionScript не управляет потоками напрямую. Скрипт SWF может изменять
`_root.clip._x`; после выполнения AVM1 хост должен создать снимок состояния
сцены и только затем запустить параллельную растеризацию.

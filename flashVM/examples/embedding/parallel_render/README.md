# Parallel rendering / Параллельная отрисовка

English: `main.c` is a C89 embedding example that fills 64 RGBA pixels using
four native workers. The callback receives a pixel index and writes only to
that pixel. Compile using the CMake target `flashvm_examples_embedding_parallel_render_main`.
This is a host-side demonstration, not Flash MovieClip rasterization.

Русский: `main.c` — пример встраивания на C89. Четыре потока заполняют
64 пикселя RGBA, каждый поток пишет в независимые ячейки буфера.
Сборка: цель CMake `flashvm_examples_embedding_parallel_render_main`.
Это пример задач хоста, а не полноценная отрисовка Flash MovieClip.

## ActionScript 1/2 note / Примечание для ActionScript 1/2

Flash ActionScript does not control worker threads directly. An ordinary SWF
might update `_root.clip._x` in its frame script; after AVM1 advances, the host
must snapshot the display state before scheduling parallel rasterization.

ActionScript не управляет потоками напрямую. Скрипт SWF может изменять
`_root.clip._x`; после выполнения AVM1 хост должен создать снимок состояния
сцены и только затем запустить параллельную растеризацию.

<!-- example-catalog-api:start -->
## Назначение и особенности

Передаёт независимые render jobs исполнителю, используя вызывающий поток для одного задания и native-потоки для остальных. Показывает разделение callback/userdata и синхронное завершение пакета работ.

**Текущая CMake-цель:** `flashvm_examples_embedding_parallel_render_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_render_jobs_run`](../../../include/flashvm/render_jobs.h#L25) | [RENDER_THREADS.md#русский](../../../doc/RENDER_THREADS.md#русский) — Параллельные задания рендеринга. |

[Описание в общем каталоге](../../README.md#example-embedding-parallel_render).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../../../include/flashvm/render_jobs.h#L25) |
<!-- source-links:end -->

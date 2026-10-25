# Render jobs / Потоки отрисовки

## English

`fvm_render_jobs_run(job_count, workers, callback, userdata)` distributes
independent host-provided rendering jobs over native worker threads. It blocks
until all jobs complete. `workers=0` runs synchronously. Jobs may run in any
order. A return value of `-1` means invalid input, allocation failure, or
thread creation failure; any successfully started jobs are still joined.

The OS bridge is selected at build time from `ports/windows` (Win32 threads),
`ports/linux`, `ports/mac`, or `ports/android` (POSIX threads). Disable with
`-DFVM_ENABLE_RENDER_JOBS=OFF` for unsupported targets. This API is C89-compatible
and does not require C++11 or STL in public headers.

**Important:** this is a parallel job executor, not yet a MovieClip rasterizer.
The current FlashVM does not have a complete display list or Cairo/AGG renderer.
The host must prepare immutable snapshots of scene geometry, divide work into
independent pixel regions or off-screen surfaces, and composite results in
Flash display-list order. Never call AVM1 or mutate Flash objects from workers.
Each callback must write to disjoint memory (or synchronize externally).
Callbacks must not throw exceptions. The caller must keep userdata alive until
completion. See `examples/embedding/parallel_render/`.

## Русский

`fvm_render_jobs_run(количество, потоки, функция, данные)` распределяет
независимые задания отрисовки между рабочими потоками и ждёт их завершения.
При `потоки=0` выполнение синхронное. Порядок заданий не гарантируется.
Код `-1` означает неверные аргументы, ошибку выделения памяти или запуска
потока; уже запущенные потоки обязательно дожидаются завершения.

Переходники находятся в `ports/windows` (Win32), `ports/linux`, `ports/mac`
и `ports/android` (POSIX). Выключение: `-DFVM_ENABLE_RENDER_JOBS=OFF`.
Публичный интерфейс совместим с C89, ядро — с C++98.

**Ограничение:** это исполнитель параллельных задач, а не готовый растеризатор
MovieClip. Display list и рендерер Cairo/AGG ещё не готовы. Хост должен
создать неизменяемый снимок сцены, разделить отрисовку на независимые области
или отдельные поверхности и объединить их в порядке слоёв Flash.
Нельзя обращаться к AVM1 и менять объекты Flash из рабочих потоков.
Память вывода заданий не должна пересекаться без синхронизации.
Данные должны оставаться действительными до возврата функции.
Пример: `examples/embedding/parallel_render/`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_render_jobs_run` | function | [include/flashvm/render_jobs.h:25](../include/flashvm/render_jobs.h#L25) |
<!-- source-links:end -->

# Проверка SWL

```sh
cmake -S linker -B build/linker
cmake --build build/linker
ctest --test-dir build/linker --output-on-failure
```

96 проверок подпроцессов не требуют компилятора, Coco/R или VM. Python stdlib
независимо проверяет объектный заголовок/checksum, SWF теги, RECT/shape edges,
bitmap matrix/JPEG bytes, все 16 PCM-профилей, exports, placement и sound events.
Неверные объекты/архивы/ресурсы не портят существующий выход. [C embedding test](tests/embed.c)
проверяет частичный отказ архива с откатом и вывод после отказа callback.
Опционально `FIR_FLASHVM_RUNNER` задаёт host для загрузки SWF и исполнения двух
DoAction в порядке входов. [Сквозной C-пример](examples/ide/README.ru.md).
В обычной и ASan/UBSan-сборке тесты прошли; ограничения санитайзеров и полная
сводка — [FIR TESTING](../fir/v1/doc/TESTING.ru.md). Проверка загрузки не является
тестом физического отображения или аудиоустройства.

# Text example / Пример текстовых функций

Build with the top-level CMake project, then run `flashvm_examples_embedding_text_main`.
`main.c` demonstrates the real C89 API; `example.as` is a conceptual AS2
comparison, **not** an automatically registered native API.

Соберите основной проект через CMake и запустите `flashvm_examples_embedding_text_main`.
Файл `main.c` использует действующий C89 API; `example.as` служит сравнением
с ActionScript 2.0, но не является исполняемым интеграционным тестом.

<!-- example-catalog-api:start -->
## Назначение и особенности

Проверяет UTF-8 и выполняет поиск регулярным выражением через C API. Позволяет проверить обработку строки до передачи в VM; AS2-исходник рядом относится к отдельному script-сценарию.

**Текущая CMake-цель:** `flashvm_examples_embedding_text_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_regex_search`](../../../include/flashvm/regex.h) | [TEXT_API.md](../../../doc/TEXT_API.md) — Регулярные выражения. |
| [`fvm_utf8_validate`](../../../include/flashvm/utf8.h) | [TEXT_API.md](../../../doc/TEXT_API.md) — Проверка и обработка UTF-8. |

[Описание в общем каталоге](../../README.md#example-embedding-text).
<!-- example-catalog-api:end -->

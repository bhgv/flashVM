# Завершение архитектурных проверок CI

Copyright (c) 2026 bhgv

[English](CI-NATIVE.md) · [Workflow](../../.github/workflows/native-disvm.yml)

Native workflow позволяет работающей master-матрице завершиться при публикации
следующего коммита. RV64/QEMU может работать дольше одного шага разработки; отмена
при каждом push многократно мешала получить полный архитектурный результат.

Branch concurrency group хранит один running запуск и последнюю pending работу.
GitHub может заменить старый pending запуск новым. Эта политика не отменяет уже
работающий master; другие refs сохраняют отмену. Наличие в очереди не означает
успешную проверку: смотрите результат точного проверяемого коммита.

Выбор workloads, timeouts, настройки sanitizer, native execution profile и критерии
успеха не меняются. Обязательны native OS owners без global stop/exclusive.
RV64 userspace/QEMU execution не объявляется bare-metal либо whole OS проверкой.

Изменение разобрано как YAML и проверено на сохранение всех jobs и существующей
branch concurrency group. Его execution/queue status сообщается отдельно от
локальных runtime tests.

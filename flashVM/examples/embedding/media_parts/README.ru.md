# Части крупного объекта и компактное управление

Исполняемый C89-пример без VM, SDL, декодеров и сокетов. [Контракт](../../../doc/MEDIA_SLOTS.ru.md).
[English](README.md). [main.c](main.c), [limits.h](limits.h), [каталог](../../README.md#example-embedding-media_parts).

Файл из трёх частей упаковывается в один буфер. Средний слот полностью разрушен,
включая поле длины; последний остаётся читаемым. Получатель перечисляет только
среднюю недостающую часть, повторяет её и сверяет полный файл. В live-режиме
части разрешено пропустить по deadline хоста: missing не формирует повторы.
Отдельное мелкое сообщение управления использует обычную запись переменной
длины без заполнения до 1024 байт. Фиксированные слоты — только opt-in bulk-профиль.

Все массивы и receiver принадлежат main. Callback синхронный, без выделений,
блокировок и передачи указателей другому потоку. Piece.data заимствует слот до
его изменения; после accept байты принадлежат буферу клиента. Пропуски в буфере
не инициализируются и не считаются готовыми данными. После полного получения
проверяется каждый байт. GC roots, SDK shutdown и освобождения библиотеки нет.

Реальная программа должна согласовать identity/tag/profile через доверенный
канал, реализовать TCP/UDP transport, pacing, лимит повторов и часы/deadline.
Пример не утверждает, что аудио/видео декодируется или воспроизводится.

```sh
cmake -S flashVM -B build-media -DFVM_BUILD_EXAMPLES=ON
cmake --build build-media --target flashvm_examples_embedding_media_parts_main
./build-media/flashvm_examples_embedding_media_parts_main
```

Из папки компонента используйте `cmake -S .`. Нужны C89 и CMake; линковка только с flashvm_srecord.

Expected / ожидается: `PASS compact control, bulk batch recovery, file retry and live no-retry policy`.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L66) | Мелкие команды кодируются с фактической переменной длиной; выход клиента, атомарные LIMIT/INVALID, без блокировки. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_sslot_batch`](../../../include/flashvm/sslot.h#L40) | Независимо проверяет известные границы; view заимствует input, без выделений/I/O; неполный пакет отвергается до callback. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_write`](../../../include/flashvm/smedia.h#L64) | Кодирует точную часть крупного объекта в слот клиента; metadata/extent INVALID, ёмкость LIMIT; ошибка не меняет выход. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_read`](../../../include/flashvm/smedia.h#L69) | Проверяет слот перед выдачей заимствованных байтов немедленному потребителю; чужой tag UNKNOWN, формат INVALID, checksum CHECKSUM. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_receiver_init`](../../../include/flashvm/smedia.h#L74) | Заимствует буфер объекта и копирует согласованную identity; без инициализации байтов/выделений; малая ёмкость LIMIT сохраняет старое состояние. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_accept`](../../../include/flashvm/smedia.h#L82) | Проверяет и копирует подходящую часть; точный дубликат идемпотентен, смена identity/байтов CONFLICT; ошибка сохраняет buffer/state/complete. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_missing`](../../../include/flashvm/smedia.h#L89) | Перечисляет FILE-пропуски по возрастанию для политики хоста; LIVE возвращает ноль. Не отправляет запросов, не блокирует; массив клиента. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
Размеры bulk-частей сначала согласуются компактными служебными S-записями tag 0.
У каждого конца отдельный exchange; предложение не меняет активную конфигурацию.
Пример запрашивает 1024/768 при локальном пределе 4096. Для больших частей
используется тот же profile API; C-тест проверяет 4096/4000 и слот 64 КиБ.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_scontrol_init`](../../../include/flashvm/scontrol.h#L74) | Копирует локальные пределы без активации формата данных. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_scontrol_propose`](../../../include/flashvm/scontrol.h#L78) | Начинает запрос с возрастающим ID; активные размеры не меняются, параллельный запрос CONFLICT. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_scontrol_write`](../../../include/flashvm/scontrol.h#L67) | Кодирует компактный tag-zero offer/ACK в буфер клиента, атомарно при ошибке. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_scontrol_read`](../../../include/flashvm/scontrol.h#L69) | Проверяет запись, копирует метаданные; неизвестная служба UNKNOWN, состояние сама не меняет. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_scontrol_respond`](../../../include/flashvm/scontrol.h#L83) | Выбирает меньшие квоты в pending; без выделений и транспорта. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_scontrol_commit`](../../../include/flashvm/scontrol.h#L90) | Активирует только коррелированный ограниченный ACK после надёжной очереди хоста; ошибка сохраняет состояние. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_write_profile`](../../../include/flashvm/smedia.h#L94) | Кодирует точную часть с согласованным profile; без выделения scratch размером со слот. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_receiver_init_profile`](../../../include/flashvm/smedia.h#L100) | Копирует согласованные profile/identity, заимствует буфер; INVALID/LIMIT сохраняют receiver. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |
| [`fvm_smedia_read_profile`](../../../include/flashvm/smedia.h#L98) | Проверяет согласованный слот и размер части; выход заимствует ввод только при OK. [Contract](../../../doc/MEDIA_SLOTS.ru.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:90](../../../include/flashvm/scontrol.h#L90) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:74](../../../include/flashvm/scontrol.h#L74) |
| `fvm_scontrol_propose` | function | [include/flashvm/scontrol.h:78](../../../include/flashvm/scontrol.h#L78) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:69](../../../include/flashvm/scontrol.h#L69) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:83](../../../include/flashvm/scontrol.h#L83) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:67](../../../include/flashvm/scontrol.h#L67) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:82](../../../include/flashvm/smedia.h#L82) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:89](../../../include/flashvm/smedia.h#L89) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:69](../../../include/flashvm/smedia.h#L69) |
| `fvm_smedia_read_profile` | function | [include/flashvm/smedia.h:98](../../../include/flashvm/smedia.h#L98) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:74](../../../include/flashvm/smedia.h#L74) |
| `fvm_smedia_receiver_init_profile` | function | [include/flashvm/smedia.h:100](../../../include/flashvm/smedia.h#L100) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:64](../../../include/flashvm/smedia.h#L64) |
| `fvm_smedia_write_profile` | function | [include/flashvm/smedia.h:94](../../../include/flashvm/smedia.h#L94) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:40](../../../include/flashvm/sslot.h#L40) |
<!-- source-links:end -->

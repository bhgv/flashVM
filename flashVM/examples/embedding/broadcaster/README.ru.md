# Пользовательский объект событий (C89 и AS2)

[English](README.md). [main.c](main.c) — исполняемый пример без окна и устройства;
[events.as](events.as) — исходный скриптовый спутник для хоста, установившего
AsBroadcaster. Каждый непустой кодовый ряд снабжён комментарием. AS-файл сам по
себе не является SWF: скомпилируйте его отдельным совместимым AS2-компилятором
в SWF6–8; команда конкретного внешнего компилятора здесь не предполагается.

Пример создаёт source и listener, устанавливает onValue, подписывает объект,
синхронно посылает 42, переносит граф через FIFO во второй контекст,
вызывает копию с 84 и снимает исходную подписку. Подходит для прикладных уведомлений
поверх любого frontend. Не читает устройства и не заменяет Key/Mouse/Selection.

## Сборка и ожидаемый результат

Требуются C89/C++98 toolchain, CMake и обычная сборка FlashVM; оконный SDK,
сеть, разрешения и физическое устройство не требуются. Из каталога flashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_broadcaster_main -j2
./build/flashvm_examples_embedding_broadcaster_main
```

```text
onValue: 42
onValue: 84
```

## Выполнение, владение и ограничения

Все вызовы синхронны в одном потоке владельца VM. Контекст владеет объектами;
pin защищают частичное построение и позднее снятие подписки. Живой Array source
удерживает listener до remove/reset/утраты достижимости. Callback не сохраняет
заимствованные self/args/out. Очистка снимает только полученные pin и разрушает
контекст перед runtime; оставшаяся после ошибки подписка исчезает при разрушении.
Ошибки идут в stderr, код процесса 1; успех — 0. Нехватка памяти передаётся, а не
заменяется assert. Контракт снимка, порядок изменений, предел 4096, версии и
открытые условия описаны в [руководстве](../../../doc/AS_BROADCASTER.ru.md).
Это учебный хост, не плеер, компилятор или приёмка исторической совместимости.


Второй контекст имеет собственную стандартную библиотеку и отдельно регистрирует
код receive под тем же стабильным именем. Машинный код и адреса объектов не
передаются. Namespace AsBroadcaster получателю не нужен: импортированы готовые
методы и живой Array. Импортированный корень закрепляется перед GC; после
рассылки снимается pin, освобождается очередь и разрушается второй контекст.
Это последовательный обмен между двумя кучами одного потока; межпоточный тест
из этого примера не следует. Файлы/процессы/сеть проверяются отдельными fixtures.

## Каждое используемое C API

| API | Параметры, действие и владение |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | config: задать ABI по умолчанию; ресурсы не создаются. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | config и выход runtime: создать внешнего владельца; разрушить после контекста. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | runtime и выход context: создать отдельную управляемую кучу. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L20) | context: синхронно установить необходимые Object/Array с выделением памяти. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_standard_broadcaster_install`](../../../include/flashvm/broadcaster.h#L13) | context: явно установить пространство имён скрипта, сохраняя конфликты. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_broadcaster_initialize`](../../../include/flashvm/broadcaster.h#L19) | context, source того же контекста: после проверки привязок атомарно установить методы и живой Array. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_object_create`](../../../include/flashvm/value.h#L62) | context, выход object: создать принадлежащий контексту объект; указатель заимствован. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L48) | context, object: удерживать при построении/дальнейшем доступе; успешно полученный pin снять. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_object_define_method_rooted`](../../../include/flashvm/value.h#L115) | context, listener, имя, callback, userdata, root: установить обработчик с this; здесь захват нулевой. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_value_undefined`](../../../include/flashvm/value.h#L40) | выход value: задать все поля перед записью объекта; без выделения памяти. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_value_string`](../../../include/flashvm/value.h#L46) | выход value, статическая UTF-8: заимствовать имя на время синхронного вызова. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | выход value, double: скопировать число; без управляемого выделения памяти. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_object_call`](../../../include/flashvm/value.h#L126) | context, source, метод, argc/argv, out: синхронные add/broadcast/remove; входы заимствованы до возврата. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L54) | context, object: снять один полученный корень после последнего доступа. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | context: освободить кучу, подписки и пространство имён; заимствованные указатели перестают действовать. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | runtime: освободить внешнего владельца после разрушения контекста. [Guide](../../../doc/AS_BROADCASTER.ru.md#установка-и-владение). |
| [`fvm_serialization_register_callback`](../../../include/flashvm/serialization.h#L30) | context, стабильное имя, callback: привязать локальный код в каждой куче; userdata не копируется. [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |
| [`fvm_channel_create`](../../../include/flashvm/channel.h#L46) | выход channel: получить один неограниченный FIFO; освободить после получения. [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |
| [`fvm_channel_send_value`](../../../include/flashvm/transfer.h#L22) | исходный context, очередь, root, blocking=0: скопировать граф до отправки; ссылки на исходную кучу не удерживаются. [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |
| [`fvm_channel_receive_value`](../../../include/flashvm/transfer.h#L25) | получатель, очередь, выход, blocking=0: восстановить сообщение в куче получателя; пустая открытая очередь даёт [FVM_NOT_FOUND](../../../include/flashvm/flashvm.h#L47). [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L44) | получатель, необязательный счётчик freed=NULL: собрать недостижимое при закреплённом импортированном корне. [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |
| [`fvm_channel_release`](../../../include/flashvm/channel.h#L70) | queue: освободить handle после остановки операций; последний владелец удаляет оставшиеся сообщения. [Guide](../../../doc/AS_BROADCASTER.ru.md#перенос-и-плеер-v0616). |

## Callback и значения

[`FVM_Config`](../../../include/flashvm/flashvm.h#L51) хранится у хоста; [`FVM_Runtime`](../../../src/core/runtime.hpp#L175) и [`FVM_Context`](../../../src/core/runtime.hpp#L247) — приобретённые
владельцы. [`FVM_Object`](../../../src/core/runtime.hpp#L122) заимствован и защищается pin/достижимостью. [`FVM_Value`](../../../include/flashvm/value.h#L24)
содержит тег: [`FVM_VALUE_OBJECT`](../../../include/flashvm/value.h#L21) выбирает object, [`FVM_VALUE_NUMBER`](../../../include/flashvm/value.h#L19) — number.
[`FVM_Result`](../../../include/flashvm/flashvm.h#L40) передаёт статус без скрытой замены ошибки.

У receive аргумент context — контекст-владелец, self — зарегистрированный
слушатель, count — количество payload без имени события, args — заимствованный
массив count значений до возврата. out — место для результата, здесь undefined;
data — указатель userdata из регистрации, здесь NULL. Callback выполняется
синхронно и не сохраняет аргументы или указатели за пределами вызова.

## Скриптовые методы

| API | Назначение |
| --- | --- |
| AsBroadcaster.initialize(source) | Установить/сбросить методы и живой Array пользовательского объекта. |
| source.addListener(observer) | Сильно удерживать идентичность получателя; возвращает true при успехе. |
| source.broadcastMessage("onValue", 42) | Вызвать onValue(42) на снимке слушателей; this — observer. |
| source.removeListener(observer) | Снять подписку, вернуть признак удаления. |
| trace(value) | Печать из внешнего хоста скрипта; вывод C-примера выполняет printf. |

Контракты и источник Adobe: [руководство](../../../doc/AS_BROADCASTER.ru.md#методы-скрипта-и-рассылка).

Registration may return [`FVM_ERROR`](../../../include/flashvm/flashvm.h#L43) when the shared execution budget is exhausted; the live list remains unchanged. See [work limits](../../../doc/AS_BROADCASTER.md#registration-work-limits-v0617) / [ограничение работы](../../../doc/AS_BROADCASTER.ru.md#ограничение-работы-регистрации-v0617).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broadcaster_initialize` | function | [include/flashvm/broadcaster.h:19](../../../include/flashvm/broadcaster.h#L19) |
| `fvm_channel_create` | function | [include/flashvm/channel.h:46](../../../include/flashvm/channel.h#L46) |
| `fvm_channel_receive_value` | function | [include/flashvm/transfer.h:25](../../../include/flashvm/transfer.h#L25) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:70](../../../include/flashvm/channel.h#L70) |
| `fvm_channel_send_value` | function | [include/flashvm/transfer.h:22](../../../include/flashvm/transfer.h#L22) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../../../include/flashvm/flashvm.h#L51) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `FVM_Context` | struct | [src/core/runtime.hpp:247](../../../src/core/runtime.hpp#L247) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../../../include/flashvm/flashvm.h#L43) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../../../include/flashvm/gc.h#L44) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../../../include/flashvm/flashvm.h#L47) |
| `FVM_Object` | struct | [src/core/runtime.hpp:122](../../../src/core/runtime.hpp#L122) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../../../include/flashvm/value.h#L126) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_object_define_method_rooted` | function | [include/flashvm/value.h:115](../../../include/flashvm/value.h#L115) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../../../include/flashvm/flashvm.h#L40) |
| `FVM_Runtime` | struct | [src/core/runtime.hpp:175](../../../src/core/runtime.hpp#L175) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_serialization_register_callback` | function | [include/flashvm/serialization.h:30](../../../include/flashvm/serialization.h#L30) |
| `fvm_standard_broadcaster_install` | function | [include/flashvm/broadcaster.h:13](../../../include/flashvm/broadcaster.h#L13) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../../../include/flashvm/value.h#L24) |
| `FVM_VALUE_NUMBER` | enum constant | [include/flashvm/value.h:19](../../../include/flashvm/value.h#L19) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
| `FVM_VALUE_OBJECT` | enum constant | [include/flashvm/value.h:21](../../../include/flashvm/value.h#L21) |
| `fvm_value_string` | function | [include/flashvm/value.h:46](../../../include/flashvm/value.h#L46) |
| `fvm_value_undefined` | function | [include/flashvm/value.h:40](../../../include/flashvm/value.h#L40) |
<!-- source-links:end -->

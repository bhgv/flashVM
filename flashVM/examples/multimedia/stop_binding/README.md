# Prepared-output stop binding / Остановка подготовленного вывода

Исполняемый C89 tutorial: native voice уже подготовлен, но ни одного PCM write
ещё не было. Реальный стандартный stopAllSounds вызывает context binding и
сбрасывает pending state. Физическое устройство не открывается. Для настоящей
очереди/микшера см. [плеер и SWF03](../../player/animation_audio/README.md).
Это модель управления, не заявление об аппаратном аудиовыводе.

Runnable C89 ownership/control example. The host owns native userdata; the VM owns
only the registration. Close detaches without another callback. No managed pointer,
assertion, audio permission, SDK or hidden file write is needed.

```sh
cmake -S flashVM -B /tmp/flashvm-build -DFVM_BUILD_EXAMPLES=ON
cmake --build /tmp/flashvm-build --target flashvm_examples_multimedia_stop_binding_main
/tmp/flashvm-build/flashvm_examples_multimedia_stop_binding_main --demo
```

Expected stdout / ожидаемый вывод: `pending=0 stops=1`.
Без аргументов usage/exit0, неверные аргументы exit2, API errors exit1, успех exit0.
Работает и с NO_MEDIA/NO_GRAPHICS. Каждый непустой C-ряд прокомментирован;
[main.c](main.c) показывает последовательность acquisition/control/detach/cleanup.

Native callback должен быть идемпотентным: каждый stop-all и context destruction
снова вызывает live binding. Ошибка сохраняет регистрацию для retry. Close не
вызывает callback, поэтому возможен после потери устройства; реальный host
останавливает/закрывает своё устройство отдельно. Userdata живёт до detach/
окончания context destruction. API только для owning thread; reentry и GC внутри
callback запрещены. Сам пример не сохраняет managed objects и не требует GC pin.
Установленный stopAllSounds — обычный standard object API, не отдельная имитация.

| Demonstrated API | Individual behavior and contract |
| --- | --- |
| [`fvm_audio_stop_binding_create`](../../../include/flashvm/audio_stop.h#L14) | Allocate/register a borrowed host callback; no invocation or ownership of userdata. [Create](../../../doc/AUDIO_STOP_BINDINGS.md#create). |
| [`fvm_audio_stop_binding_close`](../../../include/flashvm/audio_stop.h#L17) | Detach without callback/allocation; invalidate the handle, null allowed. [Close](../../../doc/AUDIO_STOP_BINDINGS.md#close). |
| [`fvm_call_bytes`](../../../include/flashvm/value.h#L98) | Synchronously invoke the real stopAllSounds global with no arguments. [API index](../../../doc/API_INDEX.md). |
| [`fvm_standard_install`](../../../include/flashvm/standard.h#L19) | Install standard globals before invoking stopAllSounds. [API index](../../../doc/API_INDEX.md). |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62) | Initialize supported runtime configuration without acquiring a VM. [Embedding](../../../doc/API_INDEX.md). |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64) | Own the native runtime until every child context is destroyed. [Embedding](../../../doc/API_INDEX.md). |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68) | Create one isolated VM owning its stop registrations. [Embedding](../../../doc/API_INDEX.md). |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70) | Attempt remaining controls, invalidate context-owned handles; userdata must still live. [Teardown](../../../doc/AUDIO_STOP_BINDINGS.md#ownership-gc-threading-and-transport). |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) | Release the outer native runtime after context cleanup. [Embedding](../../../doc/API_INDEX.md). |

[RU contracts](../../../doc/AUDIO_STOP_BINDINGS.ru.md) /
[Catalog](../../README.md#example-multimedia-stop_binding).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_stop_binding_close` | function | [include/flashvm/audio_stop.h:17](../../../include/flashvm/audio_stop.h#L17) |
| `fvm_audio_stop_binding_create` | function | [include/flashvm/audio_stop.h:14](../../../include/flashvm/audio_stop.h#L14) |
| `fvm_call_bytes` | function | [include/flashvm/value.h:98](../../../include/flashvm/value.h#L98) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../../../include/flashvm/standard.h#L19) |
<!-- source-links:end -->

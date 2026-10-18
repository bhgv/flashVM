# ActionTrace и обработчик хоста / Host trace sink

В v0.90 ActionTrace (0x26) преобразует значение в строку, вызывает
зарегистрированный обработчик и снимает значение со стека. Имя глобальной
функции trace на этот Action-код не влияет. Преобразование выполняется
и при отключённом обработчике, сохраняя побочные эффекты toString.

[`fvm_avm1_set_trace_rooted`](../include/flashvm/avm1.h#L43) принимает обычный [FVM_NativeFunction](../include/flashvm/value.h#L37), userdata
и необязательный захват из того же контекста. Обработчик получает глобальный
receiver и один STRING-аргумент с полным размером; возвращаемое значение
игнорируется, код ошибки распространяется. Захват удерживается GC до замены
или отключения. Нельзя уничтожать контекст из обработчика. Заимствованные
текст и аргументы действуют во время вызова; для сохранения нужна копия.

ActionTrace converts and consumes one operand and delivers one counted STRING
to a synchronous host sink. Global `trace` bindings do not intercept the opcode.
Conversion still occurs with delivery disabled. The API retains an optional
same-context capture, validates replacement before publishing it, and permits
a sink to replace or disable itself. A native collector barrier covers delivery;
callback errors and C++ exceptions propagate through the interpreter boundary.
The callback result is ignored. Disable with three NULL arguments after context.
This adds functions only; public value layout ABI remains 3 and graph format 8.
The host sink belongs to its context and is not serialized as a VM object.

Regression avm1_trace covers capture survival/release, full NUL output, disabled
conversion side effects, global shadowing, wrong-context rejection, sink errors,
allocation exceptions, replacement during delivery and stack underflow.
This enables output-based SWF reference checks but does not establish full Flash
trace compatibility: version-specific toString, player fallback text after
coercion exceptions, legacy encodings and display-object formatting remain open.
Эталонные строки при исключении и исторические преобразования пока открыты.
Primary comparison: [pinned Ruffle ActionTrace](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_set_trace_rooted` | function | [include/flashvm/avm1.h:43](../include/flashvm/avm1.h#L43) |
| `FVM_NativeFunction` | callback typedef | [include/flashvm/value.h:37](../include/flashvm/value.h#L37) |
<!-- source-links:end -->

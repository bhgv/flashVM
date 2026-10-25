/* FlashVM: Embedding/ActionScript tutorial: file channels.
* File: examples/actionscript/file_channels.as.
*/

// Host opts in with fvm_extensions_install(context).
// Corresponds to <"file", "flashvm-state.bin", "w">.
// Extended <...> source literals still require an extension-aware compiler.
/* Объявляем output и сохраняем начальное значение. */
var output = channel.create("file", "flashvm-state.bin", "w"); // Объявляем output и сохраняем начальное значение.
/* Объявляем state и сохраняем начальное значение. */
var state = new Object(); // Объявляем state и сохраняем начальное значение.
/* Сохраняем вычисленное значение: state.self. */
state.self = state; // Сохраняем вычисленное значение: state.self.
/* Сохраняем вычисленное значение: state.description. */
state.description = "Persistent object graph"; // Сохраняем вычисленное значение: state.description.
/* Вызываем output.send: записываем значение в канал.
* state — передаваемое значение. */
output.send(state); // Вызываем output.send: записываем значение в канал. state — передаваемое значение.
/* Вызываем output.close: закрываем выходной канал после отправки. */
output.close(); // Вызываем output.close: закрываем выходной канал после отправки.

/* Объявляем input и сохраняем начальное значение. */
var input = channel.create("file", "flashvm-state.bin", "r"); // Объявляем input и сохраняем начальное значение.
/* Объявляем restored и сохраняем начальное значение. */
var restored = input.receive(); // Объявляем restored и сохраняем начальное значение.
/* Объявляем finished и сохраняем начальное значение. */
var finished = input.tryReceive(); // undefined at EOF; receive() reports an error.
/* Вызываем input.close: закрываем входной канал. */
input.close(); // Вызываем input.close: закрываем входной канал.

// Default rw preserves the file and appends new messages. Reading starts at zero.
/* Объявляем append и сохраняем начальное значение. */
var append = channel.create("file", "flashvm-state.bin"); // Объявляем append и сохраняем начальное значение.
/* Вызываем append.send: дописываем запись в файловый канал.
* "next record" — передаваемое значение. */
append.send("next record"); // Вызываем append.send: дописываем запись в файловый канал. "next record" — передаваемое значение.
/* Вызываем append.close: закрываем файловый канал после записи. */
append.close(); // Вызываем append.close: закрываем файловый канал после записи.

// Primitive metadata follows assignment. Native hooks are registered by the host.
/* Объявляем numberWithMetadata и сохраняем начальное значение. */
var numberWithMetadata = metatable.attach(10, hostMetatable); // Объявляем numberWithMetadata и сохраняем начальное значение.
/* Объявляем metadata и сохраняем начальное значение. */
var metadata = metatable.get(numberWithMetadata); // Объявляем metadata и сохраняем начальное значение.

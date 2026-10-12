/* FlashVM: Embedding/ActionScript tutorial: file channels.
 * File: examples/actionscript/file_channels.as.
 */

// Host opts in with fvm_extensions_install(context).
// Corresponds to <"file", "flashvm-state.bin", "w">.
// Extended <...> source literals still require an extension-aware compiler.
/* Объявляем output и сохраняем начальное значение. */
var output = channel.create("file", "flashvm-state.bin", "w");
/* Объявляем state и сохраняем начальное значение. */
var state = new Object();
/* Сохраняем вычисленное значение: state.self. */
state.self = state;
/* Сохраняем вычисленное значение: state.description. */
state.description = "Persistent object graph";
/* Выполняем вызов output.send с указанными аргументами. */
output.send(state);
/* Выполняем вызов output.close с указанными аргументами. */
output.close();

/* Объявляем input и сохраняем начальное значение. */
var input = channel.create("file", "flashvm-state.bin", "r");
/* Объявляем restored и сохраняем начальное значение. */
var restored = input.receive();
/* Объявляем finished и сохраняем начальное значение. */
var finished = input.tryReceive(); // undefined at EOF; receive() reports an error.
/* Выполняем вызов input.close с указанными аргументами. */
input.close();

// Default rw preserves the file and appends new messages. Reading starts at zero.
/* Объявляем append и сохраняем начальное значение. */
var append = channel.create("file", "flashvm-state.bin");
/* Выполняем вызов append.send с указанными аргументами. */
append.send("next record");
/* Выполняем вызов append.close с указанными аргументами. */
append.close();

// Primitive metadata follows assignment. Native hooks are registered by the host.
/* Объявляем numberWithMetadata и сохраняем начальное значение. */
var numberWithMetadata = metatable.attach(10, hostMetatable);
/* Объявляем metadata и сохраняем начальное значение. */
var metadata = metatable.get(numberWithMetadata);

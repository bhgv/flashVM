// Host opts in with fvm_extensions_install(context).
// Corresponds to <"file", "flashvm-state.bin", "w">.
// Extended <...> source literals still require an extension-aware compiler.
var output = channel.create("file", "flashvm-state.bin", "w");
var state = new Object();
state.self = state;
state.description = "Persistent object graph";
output.send(state);
output.close();

var input = channel.create("file", "flashvm-state.bin", "r");
var restored = input.receive();
var finished = input.tryReceive(); // undefined at EOF; receive() reports an error.
input.close();

// Default rw preserves the file and appends new messages. Reading starts at zero.
var append = channel.create("file", "flashvm-state.bin");
append.send("next record");
append.close();

// Primitive metadata follows assignment. Native hooks are registered by the host.
var numberWithMetadata = metatable.attach(10, hostMetatable);
var metadata = metatable.get(numberWithMetadata);

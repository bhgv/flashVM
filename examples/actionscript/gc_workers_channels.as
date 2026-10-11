// Named-function extension; host must call fvm_extensions_install(context).
// Compile with a tool supporting FlashVM's documented AVM1 subset.
function echo(input) {
    sharedChannel.send(input);
    return input;
}
pipe = channel.create(4);
worker = thread.run_suspended("echo", 42, pipe);
worker.start();
message = pipe.receive();
result = worker.join();
worker = null;
collected = gc.collect();
pipe.close();
gc.mode(100);
mode = gc.state();

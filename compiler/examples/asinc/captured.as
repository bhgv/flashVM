// Parser-only FlashVM extension fixture. Executable FIR closure/task lowering
// is deliberately unavailable: the current backend must reject this source,
// rather than run its bodies synchronously or pretend to create worker threads.
var captured = 41;

// Defining an asynchronous function must not run this body. Future lowering
// must preserve the lexical cell for captured after its defining scope exits.
asinc function drawValue(value) {
    captured = value;
    trace(captured);
}

// Reaching a block posts work independently of the SWF timeline. The nested
// invocation also needs a task; its ordering/result contract remains future work.
asinc {
    drawValue(captured + 1);
}

// Expression syntax reserves a place for a task result/handle, not an integer
// invented by the syntax parser. No current executable profile accepts it.
var task = asinc {
    trace(captured);
};

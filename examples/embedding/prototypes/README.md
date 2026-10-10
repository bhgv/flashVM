# Embedding: prototype objects

`main.c` uses the implemented C API to link an instance to a prototype, then
reads an inherited property. Run the `flashvm_embedding_prototypes` target.

`example.as` demonstrates the equivalent ActionScript 2.0 behavior. **It is a
reference example, not currently executable in FlashVM**: user-defined
ActionScript functions and constructors are not yet implemented.

The VM rejects prototype cycles and prototypes from a different context.

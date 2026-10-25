/* Primitive arrays can have module-local descriptors in the current NC64 compiler. */


/* Compare a borrowed actual descriptor with a canonical primitive descriptor.
 * expected is a nonnil built-in numeric/byte type. Returns one for identical
 * descriptors or pointer-free equivalent layouts. Typed Limbo calls supply the
 * primitive semantics; this does not turn malformed bytecode into a sandbox. */
static int dvm_math_type(Type *actual, Type *expected)
{
    return actual == expected || (actual != nil && actual != H && actual->size == expected->size && actual->np == 0);
}

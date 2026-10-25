/* Tutorial constants: encoded AVM1 function x*2+2 and its independent input/output values. */
#ifndef FVM_JIT_EXAMPLE_BODY_H /* Keep tutorial bytecode definitions unique. */
#define FVM_JIT_EXAMPLE_BODY_H /* Start the example's private constant header. */
/* Preserve the original independent wire bytes while separating complete logical actions. */
#define FVM_JIT_EXAMPLE_BODY        /* Define one complete immutable AVM1 action body. */    \
    {0x96, 3, 0, 0, 'x', 0,         /* Push the terminated variable name x. */               \
     0x1c,                          /* Resolve the prepared argument through GetVariable. */ \
     0x96, 5, 0, 7, 2, 0, 0, 0,     /* Push the signed integer multiplier 2. */              \
     0x0c,                          /* Multiply the argument by the independent constant. */ \
     0x96, 5, 0, 7, 2, 0, 0, 0,     /* Push the signed integer addend 2. */                  \
     0x0a, 0x3e, 0}                 /* Add, return the result and end the original body. */
#define FVM_JIT_EXAMPLE_INPUT 20    /* Supply x through the ordinary prepared argument ABI. */
#define FVM_JIT_EXAMPLE_EXPECTED 42 /* Both execution paths must compute x*2+2. */
#endif                              /* Finish the tutorial constants. */

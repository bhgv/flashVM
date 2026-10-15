/* Tutorial constants: encoded AVM1 function x*2+2 and its independent input/output values. */
#ifndef FVM_JIT_EXAMPLE_BODY_H /* Keep tutorial bytecode definitions unique. */
#define FVM_JIT_EXAMPLE_BODY_H /* Start the example's private constant header. */
/* Push name x; GetVariable; Push integer 2; Multiply; Push integer 2; Add; Return; End. */
#define FVM_JIT_EXAMPLE_BODY                                                                       \
    {0x96, 3,    0,    0, 'x', 0, 0x1c, 0x96, 5, 0, 7,    2,    0, 0,                              \
     0,    0x0c, 0x96, 5, 0,   7, 2,    0,    0, 0, 0x0a, 0x3e, 0}
#define FVM_JIT_EXAMPLE_INPUT 20    /* x supplied through the ordinary prepared argument ABI. */
#define FVM_JIT_EXAMPLE_EXPECTED 42 /* Both execution paths must compute x*2+2. */
#endif                              /* Finish the tutorial constants. */

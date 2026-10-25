/* C89 tutorial bounds for a tiny but representable binary-scaled cubic. */
#ifndef FVM_CUBIC_EXAMPLE_CONSTANTS_H     /* Guard the example's independent constants. */
#define FVM_CUBIC_EXAMPLE_CONSTANTS_H     /* Keep tutorial limits separate from VM protocol IDs. */
#define FVM_CUBIC_EXAMPLE_EXPONENT (-600) /* Force squared derivative products below double range. */
#define FVM_CUBIC_EXAMPLE_TOLERANCE 1e-10 /* Compare normalized bounds in ordinary-size units. */
#endif                                    /* Finish the example constant guard. */

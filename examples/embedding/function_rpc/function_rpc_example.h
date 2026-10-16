/* FlashVM tutorial constants: local sample payloads and stdin coordination. */
/* Avoid repeated macro definitions in the example translation unit. */
#ifndef FVM_FUNCTION_RPC_EXAMPLE_H
/* Mark this tutorial header as included. */
#define FVM_FUNCTION_RPC_EXAMPLE_H
/* Four calls cover a value, counted exception and two plain failure statuses. */
#define FVM_FUNCTION_EXAMPLE_REQUESTS 4U
/* The sample exception includes one embedded NUL byte. */
#define FVM_FUNCTION_EXAMPLE_THROW_SIZE 3UL
/* Demonstrate a server-side numeric setting without transferring userdata. */
#define FVM_FUNCTION_EXAMPLE_BIAS 40
/* Bound the supervisor's shutdown line independently of network framing. */
#define FVM_FUNCTION_EXAMPLE_CONTROL_SIZE 16U
/* Finish the tutorial header guard. */
#endif

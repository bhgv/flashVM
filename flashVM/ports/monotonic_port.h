/* FlashVM: elapsed-time clock independent of calendar time and VM instruction encoding. */
#ifndef FVM_MONOTONIC_PORT_H
#define FVM_MONOTONIC_PORT_H /* Monotonic clock adapter guard. */
#ifdef __cplusplus
extern "C"
{
#endif


/* Return nonzero and finite nonnegative milliseconds on success; do not modify output on
     * failure. */
int fvm_port_monotonic_ms(double *output);
#ifdef __cplusplus
}
#endif
#endif

/* Monotonic 32-bit millisecond samples shared by bounded native network operations. */
#ifndef FVM_NET_CLOCK_H
#define FVM_NET_CLOCK_H
#define FVM_NET_CLOCK_MASK 0xffffffffUL /* One unsigned 32-bit tick period. */
#define FVM_NET_CLOCK_MS 1000UL         /* Milliseconds per second. */
#define FVM_NET_CLOCK_NS 1000000UL      /* Nanoseconds per millisecond. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Return 1 and publish a monotonic sample, or 0 with output unchanged. No networking startup.
     * Compute elapsed with (now-start)&MASK; callers bound durations below half one tick period. */
    int fvm_net_clock(unsigned long *out);
#ifdef __cplusplus
}
#endif
#endif

#ifndef DISVM_CLOCK_PORT_H
#define DISVM_CLOCK_PORT_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif


/* Read monotonic host milliseconds with an arbitrary epoch. Borrow output;
 * return 1 and replace it on success, 0 unchanged on invalid pointer/OS error.
 * No VM/SDL/Flash state, allocation or callbacks. Independent calls are safe
 * from native threads; resolution/suspend behavior belong to the host clock. */
int dvm_host_clock_ms(uint64_t *milliseconds);

#ifdef __cplusplus
}
#endif
#endif

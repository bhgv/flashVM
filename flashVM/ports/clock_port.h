/* FlashVM: Portable platform adapter interface or implementation: clock port.
 * File: ports/clock_port.h.
 */

#ifndef FVM_CLOCK_PORT_H
/* Define FVM CLOCK PORT H constant or compilation switch. */
#define FVM_CLOCK_PORT_H
#ifdef __cplusplus
extern "C"
{
#endif


/* Implement port clock ms for this subsystem. */
int fvm_port_clock_ms(double *);


/* Implement port calendar offset for this subsystem. */
int fvm_port_calendar_offset(double utc_ms, double *minutes_east);


/* Implement port calendar utc for this subsystem. */
int fvm_port_calendar_utc(double year,
                          double month,
                          double day,
                          double hour,
                          double minute,
                          double second,
                          double millisecond,
                          double *utc_ms);
#ifdef __cplusplus
}
#endif
#endif

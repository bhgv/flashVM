#ifndef FVM_CLOCK_PORT_H
#define FVM_CLOCK_PORT_H
#ifdef __cplusplus
extern "C"
{
#endif
    int fvm_port_clock_ms(double *);
    int fvm_port_calendar_offset(double utc_ms, double *minutes_east);
    int fvm_port_calendar_utc(double year, double month, double day, double hour, double minute,
                              double second, double millisecond, double *utc_ms);
#ifdef __cplusplus
}
#endif
#endif

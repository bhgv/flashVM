#include "clock_port.h"
#include "channel_sync.h"
#include <time.h>
#include <math.h>
#include <limits.h>
#include <string.h>
/* libc's calendar functions can share static storage. Serialize them at the
 * platform boundary, copy results immediately, and reject unsafe time_t casts. */
static int stamp(double seconds, time_t *out)
{
    double bound = 1;
    size_t bits = sizeof(time_t) * CHAR_BIT;
    size_t i;
    int signed_time = ((time_t)-1 < (time_t)0);
    for (i = 0; i < bits - (signed_time ? 1 : 0); ++i)
        bound *= 2;
    if (seconds != seconds || seconds < (signed_time ? -bound : 0) || seconds >= bound)
        return 0;
    *out = (time_t)seconds;
    return 1;
}
static double day_number(int year, int month, int day)
{
    double era;
    int yoe, mp, doy;
    year -= month <= 1;
    era = floor((double)year / 400);
    yoe = year - (int)(era * 400);
    mp = month + (month > 1 ? -2 : 10);
    doy = (153 * mp + 2) / 5 + day - 1;
    return era * 146097 + (yoe * 365 + yoe / 4 - yoe / 100 + doy) - 719468;
}
int fvm_port_calendar_offset(double utc_ms, double *east)
{
    time_t value;
    struct tm *p, local;
    double local_seconds;
    if (!east || !stamp(floor(utc_ms / 1000), &value))
        return 0;
    fvm_port_registry_lock();
    p = localtime(&value);
    if (p)
        local = *p;
    fvm_port_registry_unlock();
    if (!p)
        return 0;
    local_seconds = day_number(local.tm_year + 1900, local.tm_mon, local.tm_mday) * 86400 +
                    local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    *east = (local_seconds - floor(utc_ms / 1000)) / 60;
    return 1;
}
int fvm_port_calendar_utc(double year, double month, double day, double hour, double minute,
                          double second, double millisecond, double *utc_ms)
{
    struct tm local, *check;
    time_t value;
    double residual;
    int ok;
    if (!utc_ms || year < -1000000 || year > 1000000 || month < 0 || month > 11 || day < 1 ||
        day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
        second > 59 || millisecond < 0 || millisecond >= 1000)
        return 0;
    memset(&local, 0, sizeof(local));
    local.tm_year = (int)year - 1900;
    local.tm_mon = (int)month;
    local.tm_mday = (int)day;
    local.tm_hour = (int)hour;
    local.tm_min = (int)minute;
    local.tm_sec = (int)second;
    local.tm_isdst = -1;
    fvm_port_registry_lock();
    value = mktime(&local);
    /* -1 is also a valid instant. A successful reverse calendar conversion
     * distinguishes representability without treating 1969-12-31 as failure. */
    check = localtime(&value);
    ok = check && check->tm_year == local.tm_year && check->tm_mon == local.tm_mon &&
         check->tm_mday == local.tm_mday && check->tm_hour == local.tm_hour &&
         check->tm_min == local.tm_min && check->tm_sec == local.tm_sec;
    fvm_port_registry_unlock();
    if (!ok)
        return 0;
    residual = millisecond;
    *utc_ms = (double)value * 1000 + residual;
    return 1;
}

#include "date_state.hpp"
#include "flashvm/standard.h"
#include "../../ports/clock_port.h"
#include <cmath>
#include <cstdio>
#include <limits>
namespace
{
enum
{
    DATE_OBJECT = 34,
    DATE_CLASS = 35
};
struct Fields
{
    double year, month, day, hour, minute, second, ms, weekday;
};
double invalid()
{
    return std::numeric_limits<double>::quiet_NaN();
}
bool finite(double v)
{
    return v == v && std::fabs(v) != std::numeric_limits<double>::infinity();
}
double integer(double v)
{
    return v < 0 ? -std::floor(-v) : std::floor(v);
}
double clip(double v)
{
    return !finite(v) || std::fabs(v) > 8640000000000000.0 ? invalid() : integer(v);
}
double days(double year, double month, double day)
{
    double normalized = year + std::floor(month / 12);
    month -= std::floor(month / 12) * 12;
    if (!finite(normalized) || std::fabs(normalized) > 1000000)
        return invalid();
    long y = (long)normalized;
    y -= month <= 1;
    double era = std::floor((double)y / 400);
    long yoe = y - (long)(era * 400);
    long mp = (long)month + (month > 1 ? -2 : 10);
    double doy = (153 * mp + 2) / 5 + day - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}
double make(const Fields &f)
{
    return days(f.year, f.month, f.day) * 86400000 + f.hour * 3600000 + f.minute * 60000 +
           f.second * 1000 + f.ms;
}
Fields split(double time)
{
    Fields f;
    double day = std::floor(time / 86400000);
    long z = (long)day + 719468;
    long era = (long)std::floor((double)z / 146097);
    long doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = yoe + era * 400, doy = doe - (365 * yoe + yoe / 4 - yoe / 100),
         mp = (5 * doy + 2) / 153;
    f.day = doy - (153 * mp + 2) / 5 + 1;
    f.month = mp + (mp < 10 ? 2 : -10);
    f.year = y + (f.month <= 1);
    double rest = time - day * 86400000;
    f.hour = std::floor(rest / 3600000);
    rest -= f.hour * 3600000;
    f.minute = std::floor(rest / 60000);
    rest -= f.minute * 60000;
    f.second = std::floor(rest / 1000);
    f.ms = rest - f.second * 1000;
    f.weekday = std::fmod(day + 4, 7);
    if (f.weekday < 0)
        f.weekday += 7;
    return f;
}
bool date(FVM_Object *o)
{
    return o && o->resource_kind == DATE_OBJECT && o->resource_result_valid &&
           o->resource_result.type == FVM_VALUE_NUMBER;
}
FVM_Result number_arg(FVM_Context *c, unsigned int n, const FVM_Value *a, unsigned int i,
                      double fallback, double &v)
{
    if (i >= n)
    {
        v = fallback;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_number(c, a + i, &v);
    if (r == FVM_OK && finite(v))
        v = integer(v);
    return r;
}
FVM_Result value(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    *out = o->resource_result;
    return FVM_OK;
}
FVM_Result get(FVM_Context *, FVM_Object *o, FVM_Value *out, int field, bool utc)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    double t = o->resource_result.number;
    if (!finite(t))
    {
        fvm_value_number(out, invalid());
        return FVM_OK;
    }
    double east = 0;
    if (!utc && !fvm_port_calendar_offset(t, &east))
        return FVM_UNSUPPORTED;
    Fields f = split(t + east * 60000);
    double v = field == 0   ? f.year
               : field == 1 ? f.month
               : field == 2 ? f.day
               : field == 3 ? f.hour
               : field == 4 ? f.minute
               : field == 5 ? f.second
               : field == 6 ? f.ms
               : field == 7 ? f.weekday
               : field == 8 ? f.year - 1900
                            : -east;
    fvm_value_number(out, v);
    return FVM_OK;
}
#define GET(name, field, utc)                                                                      \
    FVM_Result name(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,                \
                    FVM_Value *out, void *)                                                        \
    {                                                                                              \
        return get(c, o, out, field, utc);                                                         \
    }
GET(get_year, 0, false)
GET(get_month, 1, false)
GET(get_date, 2, false)
GET(get_hours, 3, false) GET(get_minutes, 4, false) GET(get_seconds, 5, false)
    GET(get_milliseconds, 6, false) GET(get_day, 7, false) GET(get_old_year, 8, false)
        GET(get_offset, 9, false) GET(get_utc_year, 0, true) GET(get_utc_month, 1, true)
            GET(get_utc_date, 2, true) GET(get_utc_hours, 3, true) GET(get_utc_minutes, 4, true)
                GET(get_utc_seconds, 5, true) GET(get_utc_ms, 6, true) GET(get_utc_day, 7, true)
                    GET(get_utc_old_year, 8, true)
#undef GET
                        FVM_Result normalized_utc(const Fields &input, double &out)
{
    double local = make(input);
    if (!finite(local) || std::fabs(local) > 8640000000000000.0)
    {
        out = invalid();
        return FVM_OK;
    }
    Fields f = split(local);
    if (!fvm_port_calendar_utc(f.year, f.month, f.day, f.hour, f.minute, f.second, f.ms, &out))
        return FVM_UNSUPPORTED;
    out = clip(out);
    return FVM_OK;
}
FVM_Result set(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
               int field, bool utc)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    double t = o->resource_result.number, east = 0;
    bool valid = finite(t);
    if (!valid && field != 0 && field != 8)
    {
        fvm_value_number(out, invalid());
        return FVM_OK;
    }
    if (!valid)
        t = 0;
    if (valid && !utc && !fvm_port_calendar_offset(t, &east))
        return FVM_UNSUPPORTED;
    Fields f = split(t + east * 60000);
    double *slots[] = {&f.year, &f.month, &f.day, &f.hour, &f.minute, &f.second, &f.ms};
    unsigned int start = field == 8 ? 0 : (unsigned int)field;
    unsigned int count = field == 0                               ? 3
                         : field == 1                             ? 2
                         : field == 2 || field == 6 || field == 8 ? 1
                                                                  : 7 - start;
    FVM_Value missing;
    fvm_value_undefined(&missing);
    for (unsigned int i = 0; i < count; ++i)
    {
        if (i && i >= n)
            break;
        double v = 0;
        FVM_Result r = number_arg(c, n ? n : 1, n ? a : &missing, i, 0, v);
        if (r != FVM_OK)
            return r;
        if (!finite(v))
        {
            o->resource_result.number = invalid();
            *out = o->resource_result;
            return FVM_OK;
        }
        if (field == 8 && v >= 0 && v <= 99)
            v += 1900;
        *slots[start + i] = v;
    }
    double result = 0;
    FVM_Result r = FVM_OK;
    if (utc)
        result = clip(make(f));
    else
        r = normalized_utc(f, result);
    if (r != FVM_OK)
        return r;
    o->resource_result.number = result;
    *out = o->resource_result;
    return FVM_OK;
}
#define SET(name, field, utc)                                                                      \
    FVM_Result name(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,             \
                    FVM_Value *out, void *)                                                        \
    {                                                                                              \
        return set(c, o, n, a, out, field, utc);                                                   \
    }
SET(set_year, 0, false)
SET(set_month, 1, false)
SET(set_date, 2, false)
SET(set_hours, 3, false) SET(set_minutes, 4, false) SET(set_seconds, 5, false)
    SET(set_milliseconds, 6, false) SET(set_old_year, 8, false) SET(set_utc_year, 0, true)
        SET(set_utc_month, 1, true) SET(set_utc_date, 2, true) SET(set_utc_hours, 3, true)
            SET(set_utc_minutes, 4, true) SET(set_utc_seconds, 5, true) SET(set_utc_ms, 6, true)
#undef SET
                FVM_Result set_time(FVM_Context *c, FVM_Object *o, unsigned int n,
                                    const FVM_Value *a, FVM_Value *out, void *)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    double t = invalid();
    FVM_Result r = n ? fvm_value_to_number(c, a, &t) : FVM_OK;
    if (r == FVM_OK)
    {
        o->resource_result.number = clip(t);
        *out = o->resource_result;
    }
    return r;
}
FVM_Result text(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                void *)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    double t = o->resource_result.number;
    if (!finite(t))
        return fvm_value_string(out, "Invalid Date");
    double east = 0;
    if (!fvm_port_calendar_offset(t, &east))
        return FVM_UNSUPPORTED;
    Fields f = split(t + east * 60000);
    const char *week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int offset = (int)std::fabs(east);
    char buffer[128];
    std::sprintf(buffer, "%s %s %02d %02d:%02d:%02d GMT%c%02d%02d %ld",
                 week[(unsigned int)f.weekday], months[(unsigned int)f.month], (int)f.day,
                 (int)f.hour, (int)f.minute, (int)f.second, east < 0 ? '-' : '+', offset / 60,
                 offset % 60, (long)f.year);
    FVM_Value v;
    fvm_value_string(&v, buffer);
    return fvm_value_to_string(c, &v, out);
}
FVM_Result components(FVM_Context *c, unsigned int n, const FVM_Value *args, bool local,
                      double &out)
{
    Fields f;
    double *fields[] = {&f.year, &f.month, &f.day, &f.hour, &f.minute, &f.second, &f.ms};
    double defaults[] = {invalid(), invalid(), 1, 0, 0, 0, 0};
    for (unsigned int i = 0; i < 7; ++i)
    {
        FVM_Result r = number_arg(c, n, args, i, defaults[i], *fields[i]);
        if (r != FVM_OK)
            return r;
        if (!finite(*fields[i]))
        {
            out = invalid();
            return FVM_OK;
        }
    }
    if (f.year >= 0 && f.year <= 99)
        f.year += 1900;
    if (local)
        return normalized_utc(f, out);
    out = clip(make(f));
    return FVM_OK;
}
FVM_Result utc(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *args, FVM_Value *out,
               void *)
{
    double t = 0;
    FVM_Result r = components(c, n, args, false, t);
    if (r == FVM_OK)
        fvm_value_number(out, t);
    return r;
}
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *userdata)
{
    double t = 0;
    FVM_Result r = FVM_OK;
    if (!n)
    {
        if (!fvm_port_clock_ms(&t))
            return FVM_ERROR;
        t = clip(t);
    }
    else if (n == 1)
    {
        r = fvm_value_to_number(c, a, &t);
        t = clip(t);
    }
    else
        r = components(c, n, a, true, t);
    if (r != FVM_OK)
        return r;
    FVM_Value proto;
    r = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *o = 0;
    r = fvm_object_create(c, &o);
    if (r != FVM_OK)
        return r;
    o->resource_kind = DATE_OBJECT;
    o->resource_result_valid = true;
    fvm_value_number(&o->resource_result, t);
    o->prototype = proto.object;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method methods[] = {{"getTime", value},
                          {"valueOf", value},
                          {"toString", text},
                          {"getFullYear", get_year},
                          {"getMonth", get_month},
                          {"getDate", get_date},
                          {"getHours", get_hours},
                          {"getMinutes", get_minutes},
                          {"getSeconds", get_seconds},
                          {"getMilliseconds", get_milliseconds},
                          {"getDay", get_day},
                          {"getYear", get_old_year},
                          {"getTimezoneOffset", get_offset},
                          {"getUTCFullYear", get_utc_year},
                          {"getUTCMonth", get_utc_month},
                          {"getUTCDate", get_utc_date},
                          {"getUTCHours", get_utc_hours},
                          {"getUTCMinutes", get_utc_minutes},
                          {"getUTCSeconds", get_utc_seconds},
                          {"getUTCMilliseconds", get_utc_ms},
                          {"getUTCDay", get_utc_day},
                          {"getUTCYear", get_utc_old_year},
                          {"setTime", set_time},
                          {"setFullYear", set_year},
                          {"setMonth", set_month},
                          {"setDate", set_date},
                          {"setHours", set_hours},
                          {"setMinutes", set_minutes},
                          {"setSeconds", set_seconds},
                          {"setMilliseconds", set_milliseconds},
                          {"setYear", set_old_year},
                          {"setUTCFullYear", set_utc_year},
                          {"setUTCMonth", set_utc_month},
                          {"setUTCDate", set_utc_date},
                          {"setUTCHours", set_utc_hours},
                          {"setUTCMinutes", set_utc_minutes},
                          {"setUTCSeconds", set_utc_seconds},
                          {"setUTCMilliseconds", set_utc_ms}};
} // namespace
FVM_Result fvm_date_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *proto = 0;
    FVM_Result r = fvm_object_create(c, &klass);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &proto);
    if (r != FVM_OK)
        return r;
    klass->resource_kind = DATE_CLASS;
    proto->resource_kind = DATE_OBJECT;
    proto->resource_result_valid = true;
    fvm_value_number(&proto->resource_result, invalid());
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = klass;
    globals["Date"] = v;
    proto->properties["constructor"] = v;
    v.object = proto;
    klass->properties["prototype"] = v;
    FVM_NativeEntry e;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        e.callback = methods[i].callback;
        proto->methods[methods[i].name] = e;
    }
    e.callback = utc;
    klass->methods["UTC"] = e;
    e.callback = constructor;
    e.root = klass;
    e.userdata = klass;
    constructors["Date"] = e;
    return FVM_OK;
}
FVM_Result fvm_date_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "Date", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == DATE_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
unsigned long fvm_date_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
        return 751;
    if (callback == utc)
        return 750;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return 700 + (unsigned long)i;
    return 0;
}
FVM_NativeFunction fvm_date_callback_from_id(unsigned long id)
{
    if (id == 751)
        return constructor;
    if (id == 750)
        return utc;
    return id >= 700 && id < 700 + sizeof(methods) / sizeof(methods[0]) ? methods[id - 700].callback
                                                                        : 0;
}

/* FlashVM: Built-in ActionScript objects: date.
 * File: src/standard/date.cpp.
 */

#include "date_state.hpp"
#include "../avm1/coercion.hpp"
#include "flashvm/standard.h"
#include "../../ports/clock_port.h"
#include <cmath>
#include <cstdio>
#include <limits>
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    DATE_OBJECT = 34,
    DATE_CLASS = 35,
    DATE_GENERIC_PROTOTYPE = 30 /* Loaded prototype has no native timestamp. */
};

/* Define Fields struct and its representation. */
struct Fields
{
    double year, month, day, hour, minute, second, ms, weekday;
};

/* Compute and return invalid. */
double invalid()
{
    return std::numeric_limits<double>::quiet_NaN();
}

/* Compute and return finite. */
bool finite(double v)
{
    return v == v && std::fabs(v) != std::numeric_limits<double>::infinity();
}

/* Compute and return integer. */
double integer(double v)
{
    return v < 0 ? -std::floor(-v) : std::floor(v);
}

/* Compute and return clip. */
double clip(double v)
{
    return !finite(v) || std::fabs(v) > 8640000000000000.0 ? invalid() : integer(v);
}

/* Compute and return days. */
double days(double year, double month, double day)
{
    double normalized = year + std::floor(month / 12);
    month -= std::floor(month / 12) * 12;
    if (!finite(normalized) || std::fabs(normalized) > 1000000)
        return invalid();
    long y = (long)normalized;
    y -= month <= 1;

    /* Initialization / Инициализация локального состояния. */
    double era = std::floor((double)y / 400);
    long yoe = y - (long)(era * 400);
    long mp = (long)month + (month > 1 ? -2 : 10);
    double doy = (153 * mp + 2) / 5 + day - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

/* Compute and return make. */
double make(const Fields &f)
{
    return days(f.year, f.month, f.day) * 86400000 + f.hour * 3600000 + f.minute * 60000 +
           f.second * 1000 + f.ms;
}

/* Compute and return split. */
Fields split(double time)
{

    /* Initialization / Инициализация локального состояния. */
    Fields f;
    double day = std::floor(time / 86400000);
    long z = (long)day + 719468;
    long era = (long)std::floor((double)z / 146097);
    long doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long y = yoe + era * 400, doy = doe - (365 * yoe + yoe / 4 - yoe / 100),
         mp = (5 * doy + 2) / 153;

    /* Computation / Вычисление и обновление состояния. */
    f.day = doy - (153 * mp + 2) / 5 + 1;
    f.month = mp + (mp < 10 ? 2 : -10);
    f.year = y + (f.month <= 1);
    double rest = time - day * 86400000;

    /* Computation / Вычисление и обновление состояния. */
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

/* Compute and return date. */
bool date(FVM_Object *o)
{
    return o && o->resource_kind == DATE_OBJECT && o->resource_result_valid &&
           o->resource_result.type == FVM_VALUE_NUMBER;
}

/* Compute and return number arg. */
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

/* Compute and return value. */
FVM_Result value(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;
    *out = o->resource_result;
    return FVM_OK;
}

/* Read the current value. */
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

    /* Initialization / Инициализация локального состояния. */
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
GET(get_hours, 3, false)
GET(get_minutes, 4, false)
GET(get_seconds, 5, false)
GET(get_milliseconds, 6, false)
GET(get_day, 7, false)
GET(get_old_year, 8, false)
GET(get_offset, 9, false)
GET(get_utc_year, 0, true)
GET(get_utc_month, 1, true) GET(get_utc_date, 2, true) GET(get_utc_hours, 3, true)
    GET(get_utc_minutes, 4, true) GET(get_utc_seconds, 5, true) GET(get_utc_ms, 6, true)
        GET(get_utc_day, 7, true) GET(get_utc_old_year, 8, true)
#undef GET
    /* Compute and return normalized utc. */
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

/* Update the current value. */
FVM_Result set(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
               int field, bool utc)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
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

    /* Initialization / Инициализация локального состояния. */
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

    /* Initialization / Инициализация локального состояния. */
    double result = 0;
    FVM_Result r = FVM_OK;
    if (utc)
        result = clip(make(f));
    else
        r = normalized_utc(f, result);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
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
SET(set_hours, 3, false)
SET(set_minutes, 4, false)
SET(set_seconds, 5, false)
SET(set_milliseconds, 6, false)
SET(set_old_year, 8, false)
SET(set_utc_year, 0, true)
SET(set_utc_month, 1, true)
SET(set_utc_date, 2, true)
SET(set_utc_hours, 3, true) SET(set_utc_minutes, 4, true) SET(set_utc_seconds, 5, true)
    SET(set_utc_ms, 6, true)
#undef SET
    /* Update time. */
    FVM_Result set_time(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    if (!date(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    double t = invalid();
    FVM_Result r = n ? fvm_value_to_number(c, a, &t) : FVM_OK;
    if (r == FVM_OK)
    {
        double projected = clip(t);
        /* Loaded TimeClip floors negative fractions; host callers retain truncation. */
        if (c->swf_loaded && finite(projected))
            projected = std::floor(t);
        o->resource_result.number = projected;
        *out = o->resource_result;
    }
    return r;
}

/* Format one timestamp through the existing local calendar adapter. */
FVM_Result format_time(FVM_Context *c, double t, FVM_Value *out)
{
    if (!finite(t))
        return fvm_value_string(out, "Invalid Date");
    double east = 0;
    if (!fvm_port_calendar_offset(t, &east))
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    Fields f = split(t + east * 60000);
    const char *week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int offset = (int)std::fabs(east);
    char buffer[128];
    /* Loaded Date text leaves the day unpadded, unlike the retained host spelling. */
    const char *format = c->swf_loaded ? "%s %s %d %02d:%02d:%02d GMT%c%02d%02d %ld"
                                     : "%s %s %02d %02d:%02d:%02d GMT%c%02d%02d %ld";
    std::sprintf(buffer, format,
                 week[(unsigned int)f.weekday], months[(unsigned int)f.month], (int)f.day,
                 (int)f.hour, (int)f.minute, (int)f.second, east < 0 ? '-' : '+', offset / 60,
                 offset % 60, (long)f.year);

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_string(&v, buffer);
    return fvm_value_to_string(c, &v, out);
}

/* Format native Date data; unloaded wrong receivers retain the host guard. */
FVM_Result text(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                void *)
{
    return date(o) ? format_time(c, o->resource_result.number, out) : FVM_INVALID_ARGUMENT;
}

/* Compute and return components. */
FVM_Result components(FVM_Context *c, unsigned int n, const FVM_Value *args, bool local,
                      double &out)
{

    /* Initialization / Инициализация локального состояния. */
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

/* Compute and return utc. */
FVM_Result utc(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *args, FVM_Value *out,
               void *)
{

    /* Initialization / Инициализация локального состояния. */
    double t = 0;
    FVM_Result r = components(c, n, args, false, t);
    if (r == FVM_OK)
        fvm_value_number(out, t);
    return r;
}

/* Compute and return constructor. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
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
        /* Loaded single-timestamp construction retains bounded fractional data.
         * Keep the existing range/nonfinite guard until the calendar engine is audited. */
        if (!c->swf_loaded || !finite(t) || std::fabs(t) > 8640000000000000.0)
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

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *o = 0;
    r = fvm_object_create(c, &o);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    o->resource_kind = DATE_OBJECT;
    o->resource_result_valid = true;
    fvm_value_number(&o->resource_result, t);
    o->prototype = proto.object;
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}

/* Loaded Date() returns current local time as text without coercing arguments.
 * The captured class retains the historical unloaded constructor fallback. */
FVM_Result date_call(FVM_Context *c, FVM_Object *self, unsigned int count,
                     const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!c->swf_loaded)
        return constructor(c, self, count, args, out, userdata);
    double timestamp = 0;
    if (!fvm_port_clock_ms(&timestamp))
        return FVM_ERROR;
    return format_time(c, timestamp, out);
}

/* Define Method struct and its representation. */
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
/* Convert the shared Date operand prefix once, stopping at undefined or seven values. */
FVM_Result project_arguments(FVM_Context *c, unsigned int count, const FVM_Value *args,
                             FVM_Value *numbers, unsigned int &used)
{
    used = 0;
    while (used < count && used < 7 && args[used].type != FVM_VALUE_UNDEFINED)
    {
        double number = 0;
        FVM_Result result = fvm_avm1_number(c, args + used, &number);
        if (result != FVM_OK)
            return result;
        fvm_value_number(numbers + used, number);
        ++used;
    }
    return FVM_OK;
}

/* Project loaded constructor operands; unloaded construction keeps the host contract. */
FVM_Result loaded_constructor(FVM_Context *c, FVM_Object *self, unsigned int count,
                              const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!c->swf_loaded)
        return constructor(c, self, count, args, out, userdata);
    FVM_Value numbers[7];
    unsigned int used = 0;
    FVM_Result result = project_arguments(c, count, args, numbers, used);
    return result == FVM_OK ? constructor(c, self, used, numbers, out, userdata) : result;
}

/* Date.UTC uses the same operand prefix; fewer than two projected values return undefined. */
FVM_Result loaded_utc(FVM_Context *c, FVM_Object *self, unsigned int count,
                      const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!c->swf_loaded)
        return utc(c, self, count, args, out, userdata);
    FVM_Value numbers[7];
    unsigned int used = 0;
    FVM_Result result = project_arguments(c, count, args, numbers, used);
    if (result != FVM_OK)
        return result;
    if (used < 2)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return utc(c, self, used, numbers, out, userdata);
}

/* [GC:] Numeric argument projections contain no object/string edges. The native
 * invocation retains the original operands while conversions run in source order. */
FVM_Result loaded_method(FVM_Context *c, FVM_Object *self, unsigned int count,
                         const FVM_Value *args, FVM_Value *out, void *userdata,
                         FVM_NativeFunction callback)
{
    if (!c->swf_loaded)
        return callback(c, self, count, args, out, userdata);
    if (!self || self->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Value numbers[7];
    unsigned int used = 0;
    FVM_Result result = project_arguments(c, count, args, numbers, used);
    if (result != FVM_OK)
        return result;
    if (self->resource_kind != DATE_OBJECT)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return callback(c, self, used, numbers, out, userdata);
}

/* Preserve one stable native callback identity per Date method implementation. */
#define DATE_LOADED_WRAPPER(name) \
    FVM_Result loaded_##name(FVM_Context *c, FVM_Object *self, unsigned int count, \
                             const FVM_Value *args, FVM_Value *out, void *userdata) \
    { return loaded_method(c, self, count, args, out, userdata, name); }
/* Dispatch value through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(value)
/* Dispatch text through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(text)
/* Dispatch get_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_year)
/* Dispatch get_month through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_month)
/* Dispatch get_date through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_date)
/* Dispatch get_hours through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_hours)
/* Dispatch get_minutes through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_minutes)
/* Dispatch get_seconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_seconds)
/* Dispatch get_milliseconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_milliseconds)
/* Dispatch get_day through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_day)
/* Dispatch get_old_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_old_year)
/* Dispatch get_offset through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_offset)
/* Dispatch get_utc_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_year)
/* Dispatch get_utc_month through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_month)
/* Dispatch get_utc_date through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_date)
/* Dispatch get_utc_hours through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_hours)
/* Dispatch get_utc_minutes through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_minutes)
/* Dispatch get_utc_seconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_seconds)
/* Dispatch get_utc_ms through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_ms)
/* Dispatch get_utc_day through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_day)
/* Dispatch get_utc_old_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(get_utc_old_year)
/* Dispatch set_time through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_time)
/* Dispatch set_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_year)
/* Dispatch set_month through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_month)
/* Dispatch set_date through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_date)
/* Dispatch set_hours through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_hours)
/* Dispatch set_minutes through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_minutes)
/* Dispatch set_seconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_seconds)
/* Dispatch set_milliseconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_milliseconds)
/* Dispatch set_old_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_old_year)
/* Dispatch set_utc_year through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_year)
/* Dispatch set_utc_month through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_month)
/* Dispatch set_utc_date through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_date)
/* Dispatch set_utc_hours through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_hours)
/* Dispatch set_utc_minutes through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_minutes)
/* Dispatch set_utc_seconds through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_seconds)
/* Dispatch set_utc_ms through the versioned Date argument projection. */
DATE_LOADED_WRAPPER(set_utc_ms)
#undef DATE_LOADED_WRAPPER
const Method methods[] = {{"getTime", loaded_value},
                          {"valueOf", loaded_value},
                          {"toString", loaded_text},
                          {"getFullYear", loaded_get_year},
                          {"getMonth", loaded_get_month},
                          {"getDate", loaded_get_date},
                          {"getHours", loaded_get_hours},
                          {"getMinutes", loaded_get_minutes},
                          {"getSeconds", loaded_get_seconds},
                          {"getMilliseconds", loaded_get_milliseconds},
                          {"getDay", loaded_get_day},
                          {"getYear", loaded_get_old_year},
                          {"getTimezoneOffset", loaded_get_offset},
                          {"getUTCFullYear", loaded_get_utc_year},
                          {"getUTCMonth", loaded_get_utc_month},
                          {"getUTCDate", loaded_get_utc_date},
                          {"getUTCHours", loaded_get_utc_hours},
                          {"getUTCMinutes", loaded_get_utc_minutes},
                          {"getUTCSeconds", loaded_get_utc_seconds},
                          {"getUTCMilliseconds", loaded_get_utc_ms},
                          {"getUTCDay", loaded_get_utc_day},
                          {"getUTCYear", loaded_get_utc_old_year},
                          {"setTime", loaded_set_time},
                          {"setFullYear", loaded_set_year},
                          {"setMonth", loaded_set_month},
                          {"setDate", loaded_set_date},
                          {"setHours", loaded_set_hours},
                          {"setMinutes", loaded_set_minutes},
                          {"setSeconds", loaded_set_seconds},
                          {"setMilliseconds", loaded_set_milliseconds},
                          {"setYear", loaded_set_old_year},
                          {"setUTCFullYear", loaded_set_utc_year},
                          {"setUTCMonth", loaded_set_utc_month},
                          {"setUTCDate", loaded_set_utc_date},
                          {"setUTCHours", loaded_set_utc_hours},
                          {"setUTCMinutes", loaded_set_utc_minutes},
                          {"setUTCSeconds", loaded_set_utc_seconds},
                          {"setUTCMilliseconds", loaded_set_utc_ms}};
} // namespace
/* Build constructor and prototype for date. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_date_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
               /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors,
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &functions)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *klass = 0, *proto = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, &klass);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &proto);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->resource_kind = DATE_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->resource_kind = c->swf_loaded ? DATE_GENERIC_PROTOTYPE : DATE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->resource_result_valid = !c->swf_loaded;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    if (c->swf_loaded)
        fvm_value_undefined(&proto->resource_result);
    else
        fvm_value_number(&proto->resource_result, invalid());
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_undefined(&v);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.type = FVM_VALUE_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["Date"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->properties["constructor"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    v.object = proto;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry e;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        e.callback = methods[i].callback;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        proto->methods[methods[i].name] = e;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = loaded_utc;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->methods["UTC"] = e;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = loaded_constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    e.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.userdata = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["Date"] = e;
    /* [GC:] Retain the class for unloaded function fallback and transported aliases. */
    e.callback = date_call;
    functions["Date"] = e;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Validate date. */
FVM_Result fvm_date_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "Date", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == DATE_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

/* [GC:] Prepare the unchanged invalid host Date prototype without mutation;
 * its constructor capture retains it and allocating lookups may throw bad_alloc. */
FVM_Object *fvm_date_prepare_loaded(FVM_Context *c)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("Date");
    if (ctor == c->constructors.end() || ctor->second.callback != loaded_constructor || !ctor->second.root)
        return 0;
    FVM_Object *klass = ctor->second.root;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
    if (stored == klass->properties.end() || stored->second.type != FVM_VALUE_OBJECT)
        return 0;
    FVM_Object *proto = stored->second.object;
    if (!date(proto) || proto->resource_result.number == proto->resource_result.number)
        return 0;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = proto->methods.find("getTime");
    return method != proto->methods.end() && method->second.callback == loaded_value ? proto : 0;
}

/* Map a native date callback to its stable serialization identifier. */
unsigned long fvm_date_callback_id(FVM_NativeFunction callback)
{
    if (callback == date_call)
        return 752;
    if (callback == loaded_constructor)
        return 751;
    if (callback == loaded_utc)
        return 750;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return 700 + (unsigned long)i;
    return 0;
}

/* Restore a native date callback from its stable serialization identifier. */
FVM_NativeFunction fvm_date_callback_from_id(unsigned long id)
{
    if (id == 752)
        return date_call;
    if (id == 751)
        return loaded_constructor;
    if (id == 750)
        return loaded_utc;
    return id >= 700 && id < 700 + sizeof(methods) / sizeof(methods[0]) ? methods[id - 700].callback
                                                                        : 0;
}

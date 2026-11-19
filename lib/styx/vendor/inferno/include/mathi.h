/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: include/mathi implementation and native state.
 * RU: реализация include/mathi и нативное состояние.
 */
#include "fdlibm-names.h"

/*
 * floating point control and status register masks
 */
enum
{
    INVAL = 0x0001,
    ZDIV = 0x0002,
    OVFL = 0x0004,
    UNFL = 0x0008,
    INEX = 0x0010,
    RND_NR = 0x0000,
    RND_NINF = 0x0100,
    RND_PINF = 0x0200,
    RND_Z = 0x0300,
    RND_MASK = 0x0300
};

extern double ipow10(int);


/* [standalone-contract:FPinit]
 * EN: FPinit: libmath/FPcontrol-Linux implementation and native state.
 * RU: FPinit: реализация libmath/FPcontrol-Linux и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void FPinit(void);
extern double dot(int, double *, double *);


/* [standalone-contract:FPcontrol]
 * EN: FPcontrol: libmath/FPcontrol-Linux implementation and native state.
 * RU: FPcontrol: реализация libmath/FPcontrol-Linux и нативное состояние.
 * fcr (ulong fcr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mask (ulong mask): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong FPcontrol(ulong, ulong);


/* [standalone-contract:FPstatus]
 * EN: FPstatus: libmath/FPcontrol-Linux implementation and native state.
 * RU: FPstatus: реализация libmath/FPcontrol-Linux и нативное состояние.
 * fsr (ulong fsr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mask (ulong mask): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong FPstatus(ulong, ulong);
extern void gemm(int, int, int, int, int, double, double *, int, double *, int, double, double *, int);


/* [standalone-contract:getFPstatus]
 * EN: getFPstatus: libmath/FPcontrol-Linux implementation and native state.
 * RU: getFPstatus: реализация libmath/FPcontrol-Linux и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong getFPstatus(void);


/* [standalone-contract:getFPcontrol]
 * EN: getFPcontrol: libmath/FPcontrol-Linux implementation and native state.
 * RU: getFPcontrol: реализация libmath/FPcontrol-Linux и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong getFPcontrol(void);


/* [standalone-contract:g_fmt]
 * EN: g_fmt: libmath/g_fmt implementation and native state.
 * RU: g_fmt: реализация libmath/g_fmt и нативное состояние.
 * b (register char *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * x (double x): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * echr (int echr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *g_fmt(char *, double, int);
extern int iamax(int, double *);
extern double fdim(double, double);
extern double fmax(double, double);
extern double fmin(double, double);
extern double norm2(int, double *);
extern double norm1(int, double *);


/* [standalone-contract:strtod]
 * EN: strtod: host/decimal_sync implementation and native state.
 * RU: strtod: реализация host/decimal_sync и нативное состояние.
 * text (const char *text): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * end (char **end): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern double strtod(const char *, char **);

/* fdlibm */
extern double __ieee754_acos(double);
extern double __ieee754_acosh(double);
extern double __ieee754_asin(double);
extern double asinh(double);
extern double atan(double);
extern double __ieee754_atan2(double, double);
extern double __ieee754_atanh(double);
extern double cbrt(double);
extern double ceil(double);
extern double copysign(double, double);
extern double cos(double);
extern double __ieee754_cosh(double);
extern double erf(double);
extern double erfc(double);
extern double __ieee754_exp(double);
extern double expm1(double);
extern double fabs(double);
extern int finite(double);
extern double floor(double);
extern double __ieee754_fmod(double, double);
extern double __ieee754_hypot(double, double);
extern int ilogb(double);
extern double __ieee754_j0(double);
extern double __ieee754_j1(double);
extern double __ieee754_jn(int, double);
extern double __ieee754_lgamma_r(double, int *);
extern double __ieee754_log(double);
extern double __ieee754_log10(double);
extern double log1p(double);
extern double logb(double);
extern double modf(double, double *);
extern double nextafter(double, double);
extern double __ieee754_pow(double, double);
extern double __ieee754_remainder(double, double);
extern double rint(double);
extern double scalbn(double, int);
extern double sin(double);
extern double __ieee754_sinh(double);
extern double __ieee754_sqrt(double);
extern double tan(double);
extern double tanh(double);
extern double __ieee754_y0(double);
extern double __ieee754_y1(double);
extern double __ieee754_yn(int, double);

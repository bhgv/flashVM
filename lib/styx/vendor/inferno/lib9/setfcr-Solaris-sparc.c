/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 setfcr-Solaris-sparc support.
 * RU: исходная POSIX-поддержка lib9 setfcr-Solaris-sparc.
 */
/* This code is a little awkward.  If somebody who understands Solaris
   better would tell me an idiomatic way to invoke equivalent
   behavior, I'd be grateful.    ehg@bell-labs.com  */

#include "lib9.h"

/* [standalone-contract:getfcr]
 * EN: getfcr: original POSIX lib9 setfcr-Solaris-sparc support.
 * RU: getfcr: исходная POSIX-поддержка lib9 setfcr-Solaris-sparc.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfcr(void)
{
    ulong v;

    asm("	st	%fsr, [%fp-8]");
    return v;
}

/* [standalone-contract:setfcr]
 * EN: setfcr: original POSIX lib9 setfcr-Solaris-sparc support.
 * RU: setfcr: исходная POSIX-поддержка lib9 setfcr-Solaris-sparc.
 * v (ulong v): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfcr(ulong v)
{
    ulong vv;

    vv = (getfcr() & ~FPFCR) | (v & FPFCR);
    asm("	ld	[%fp-4], %fsr");
}

/* [standalone-contract:getfsr]
 * EN: getfsr: original POSIX lib9 setfcr-Solaris-sparc support.
 * RU: getfsr: исходная POSIX-поддержка lib9 setfcr-Solaris-sparc.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfsr(void)
{
    ulong v;

    asm("	st	%fsr, [%fp-8]");
    return v;
}

/* [standalone-contract:setfsr]
 * EN: setfsr: original POSIX lib9 setfcr-Solaris-sparc support.
 * RU: setfsr: исходная POSIX-поддержка lib9 setfcr-Solaris-sparc.
 * v (ulong v): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfsr(ulong v)
{
    ulong vv;

    vv = (getfsr() & ~FPFSR) | (v & FPFSR);
    asm("	ld	[%fp-4], %fsr");
}

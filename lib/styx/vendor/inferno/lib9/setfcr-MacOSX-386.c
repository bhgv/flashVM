/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 setfcr-MacOSX-386 support.
 * RU: исходная POSIX-поддержка lib9 setfcr-MacOSX-386.
 */
/*
 * Linux 386 fpu support
 * Mimic Plan9 floating point support
 */

#include "lib9.h"

/* [standalone-contract:setfcr]
 * EN: setfcr: original POSIX lib9 setfcr-MacOSX-386 support.
 * RU: setfcr: исходная POSIX-поддержка lib9 setfcr-MacOSX-386.
 * fcr (ulong fcr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfcr(ulong fcr)
{
    __asm__("xorb	$0x3f, %%al\n\t"
            "pushw	%%ax\n\t"
            "fwait\n\t"
            "fldcw	(%%esp)\n\t"
            "popw	%%ax\n\t"
            : /* no output */
            : "al"(fcr));
}

/* [standalone-contract:getfcr]
 * EN: getfcr: original POSIX lib9 setfcr-MacOSX-386 support.
 * RU: getfcr: исходная POSIX-поддержка lib9 setfcr-MacOSX-386.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfcr(void)
{
    ulong fcr = 0;

    __asm__("pushl	%%eax\n\t"
            "fwait\n\t"
            "fstcw	(%%esp)\n\t"
            "popl	%%eax\n\t"
            "xorb	$0x3f, %%al\n\t"
            : "=a"(fcr)
            : "eax"(fcr));
    return fcr;
}

/* [standalone-contract:getfsr]
 * EN: getfsr: original POSIX lib9 setfcr-MacOSX-386 support.
 * RU: getfsr: исходная POSIX-поддержка lib9 setfcr-MacOSX-386.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfsr(void)
{
    ulong fsr = -1;

    __asm__("fwait\n\t"
            "fstsw	(%%eax)\n\t"
            "movl	(%%eax), %%eax\n\t"
            "andl	$0xffff, %%eax\n\t"
            : "=a"(fsr)
            : "eax"(&fsr));
    return fsr;
}

/* [standalone-contract:setfsr]
 * EN: setfsr: original POSIX lib9 setfcr-MacOSX-386 support.
 * RU: setfsr: исходная POSIX-поддержка lib9 setfcr-MacOSX-386.
 * fsr (ulong fsr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfsr(ulong fsr)
{
    __asm__("fclex\n\t");
}

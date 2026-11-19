/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 setfcr-MacOSX-power support.
 * RU: исходная POSIX-поддержка lib9 setfcr-MacOSX-power.
 */
/*
 * MacOSX/Darwin ppc fpu support
 * Mimic Plan9 floating point support
 */

#include "lib9.h"
#include <architecture/ppc/fp_regs.h>

/* [standalone-contract:getfcr]
 * EN: getfcr: original POSIX lib9 setfcr-MacOSX-power support.
 * RU: getfcr: исходная POSIX-поддержка lib9 setfcr-MacOSX-power.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfcr(void)
{
    ppc_fp_scr_t fpscr = get_fp_scr();
    return ((ulong *)&fpscr)[1];
}

/* [standalone-contract:getfsr]
 * EN: getfsr: original POSIX lib9 setfcr-MacOSX-power support.
 * RU: getfsr: исходная POSIX-поддержка lib9 setfcr-MacOSX-power.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong getfsr(void)
{
    ppc_fp_scr_t fpscr = get_fp_scr();
    return ((ulong *)&fpscr)[1];
}

/* [standalone-contract:setfsr]
 * EN: setfsr: original POSIX lib9 setfcr-MacOSX-power support.
 * RU: setfsr: исходная POSIX-поддержка lib9 setfcr-MacOSX-power.
 * fsr (ulong fsr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfsr(ulong fsr)
{
    ppc_fp_scr_t fpscr = get_fp_scr();
    (((ulong *)&fpscr)[1]) = fsr;
    set_fp_scr(fpscr);
}

/* [standalone-contract:setfcr]
 * EN: setfcr: original POSIX lib9 setfcr-MacOSX-power support.
 * RU: setfcr: исходная POSIX-поддержка lib9 setfcr-MacOSX-power.
 * fcr (ulong fcr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setfcr(ulong fcr)
{
    ppc_fp_scr_t fpscr = get_fp_scr();
    (((ulong *)&fpscr)[1]) = fcr;
    set_fp_scr(fpscr);
}

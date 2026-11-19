/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 pread-Nt support.
 * RU: исходная POSIX-поддержка lib9 pread-Nt.
 */
#include "lib9.h"

/* Restore the shared descriptor offset and the read error after positional I/O. */


/* [standalone-contract:pread]
 * EN: pread: original POSIX lib9 pread-Nt support.
 * RU: pread: исходная POSIX-поддержка lib9 pread-Nt.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (void *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * offset (vlong offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: long; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
long pread(int fd, void *buf, long n, vlong offset)
{
    vlong previous;
    long count;
    int saved;

    if (n < 0 || offset < 0)
    {
        errno = EINVAL;
        return -1;
    }
    previous = seek(fd, 0, 1);
    if (previous < 0 || seek(fd, offset, 0) < 0)
    {
        return -1;
    }
    count = read(fd, buf, n);
    saved = errno;
    if (seek(fd, previous, 0) < 0)
    {
        return -1;
    }
    errno = saved;
    return count;
}

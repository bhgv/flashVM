/* Copyright (c) 2026 bhgv
 * EN: Linux/POSIX hosted startup entropy adapter; pure traversal core performs no physical I/O.
 * RU: Linux/POSIX startup entropy adapter; чистое ядро traversal не выполняет physical I/O.
 */
#include "walk_boot.h"
#include "walk.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


/* EN/RU: See walk_boot.h; own and close the entropy descriptor on every path, preserving already configured IDs.
 * См. walk_boot.h; владеть и закрыть entropy descriptor при любом исходе, сохранить configured IDs.
 */
int dvm_vfs_walk_initialize(void)
{
    unsigned long domain[4];
    if (walk_domain_get(domain))
    {
        return 1;
    }
    unsigned char bytes[16];
    size_t position = 0;
    int input = open("/dev/urandom", O_RDONLY);
    if (input < 0)
    {
        return 0;
    }
    while (position < sizeof(bytes))
    {
        ssize_t count = read(input, bytes + position, sizeof(bytes) - position);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            break;
        }
        position += (size_t)count;
    }
    close(input);
    if (position != sizeof(bytes))
    {
        return 0;
    }
    for (unsigned i = 0; i < 4; i++)
    {
        domain[i] = (unsigned long)bytes[i * 4] | (unsigned long)bytes[i * 4 + 1] << 8 |
            (unsigned long)bytes[i * 4 + 2] << 16 | (unsigned long)bytes[i * 4 + 3] << 24;
    }
    return walk_domain(domain);
}

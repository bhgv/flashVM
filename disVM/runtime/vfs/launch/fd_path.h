/* Copyright (c) 2026 bhgv
 * EN: Numbered Styx files shared by launch publication and acquisition.
 * RU: Номерные файлы Styx для публикации и получения при запуске.
 */
#ifndef DVM_LAUNCH_FD_PATH_H
#define DVM_LAUNCH_FD_PATH_H
#include <stdio.h>

#define DVM_LAUNCH_FD_COUNT 32U /* Supported guest FD numbers are 0 through 31. */
#define DVM_LAUNCH_FD_PATH_SIZE 8U /* Capacity for "/.fd/31" plus terminator. */


/* EN/RU: Write the absolute publication path for number, returning 1 or 0 on invalid number.
 * Записать абсолютный путь публикации номера; вернуть 1 либо 0 для неверного номера. */
static inline int dvm_launch_fd_path(char out[DVM_LAUNCH_FD_PATH_SIZE], unsigned number)
{
    if (number >= DVM_LAUNCH_FD_COUNT)
    {
        return 0;
    }
    snprintf(out, DVM_LAUNCH_FD_PATH_SIZE, "/.fd/%u", number);
    return 1;
}

#endif

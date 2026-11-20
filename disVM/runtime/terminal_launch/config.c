/* Copyright (c) 2026 bhgv
 * EN: Validate the host's terminal launch options before touching its carrier.
 * RU: Проверка настроек терминального запуска до обращения к соединению.
 */
#include "config.h"
#include <string.h>


/* EN/RU: Parse a canonical decimal uint32 bounded by max, returning 1/0.
 * Разобрать канонический uint32 в пределах max, вернуть 1/0. */
static int number(const char *text, unsigned long max, unsigned long *out)
{
    if (!*text || (*text == '0' && text[1]))
    {
        return 0;
    }
    unsigned long value = 0;
    for (; *text; text++)
    {
        if (*text < '0' || *text > '9' || value > (max - (unsigned long)(*text - '0')) / 10)
        {
            return 0;
        }
        value = value * 10 + (unsigned long)(*text - '0');
    }
    *out = value;
    return 1;
}


/* EN/RU: Return 1 if path is the named file or lies directly below it.
 * Вернуть 1, если path совпадает с файлом или лежит под ним. */
static int covers(const char *file, const char *path)
{
    size_t length = strlen(file);
    return !strncmp(file, path, length) && (!path[length] || path[length] == '/');
}


/* EN/RU: Verify a canonical absolute non-root guest path; no lookup.
 * Проверить канонический абсолютный путь гостя, кроме корня, без I/O. */
static int guest_path(const char *path)
{
    return path[0] == '/' && path[1] && strlen(path) < DVM_NS_PATH_LIMIT &&
        strcmp(path + 1, ".") && dvm_terminal_channel_paths_valid(path + 1, path + 1) &&
        !covers("/app.dis", path) && !covers("/dep.dis", path);
}


/* EN/RU: Parse one exact mode spelling into READ/WRITE bits, or zero on refusal.
 * Преобразовать режим в права READ/WRITE, при отказе вернуть ноль. */
static unsigned grant_access(const char *mode)
{
    if (!strcmp(mode, "r"))
    {
        return FVM_ACCESS_READ;
    }
    if (!strcmp(mode, "w"))
    {
        return FVM_ACCESS_WRITE;
    }
    return !strcmp(mode, "rw") ? FVM_ACCESS_READ | FVM_ACCESS_WRITE : 0;
}


/* EN/RU: Append one unique path and ordered tag to the borrowed configuration.
 * Добавить уникальный путь и ordered-тег в заимствованную конфигурацию. */
static int grant(TerminalLaunchConfig *config, char **fields)
{
    unsigned long tag;
    unsigned access = grant_access(fields[2]);
    if (!access || !guest_path(fields[0]) || !number(fields[1], 0xffffffffUL, &tag) ||
        config->count == DVM_TERMINAL_GRANTS_MAX)
    {
        return 0;
    }
    for (size_t i = 0; i < config->count; i++)
    {
        if (tag == config->tags[i] || covers(fields[0], config->files[i].path) ||
            covers(config->files[i].path, fields[0]))
        {
            return 0;
        }
    }
    size_t index = config->count++;
    config->tags[index] = tag;
    config->files[index] = (StyxLaunchFile){fields[0], tag, access};
    return 1;
}


/* EN/RU: Append one canonical terminal file grant, preserving argv ownership.
 * Добавить выдачу канонического файла терминала, сохраняя владение argv. */
static int fd_grant(TerminalLaunchConfig *config, char **fields)
{
    unsigned access = grant_access(fields[2]);
    if (!access || !guest_path(fields[0]) || !strcmp(fields[1], ".") ||
        !dvm_terminal_channel_paths_valid(fields[1], fields[1]) || config->fd_count == STYX_LAUNCH_FDS_MAX)
    {
        return 0;
    }
    for (size_t i = 0; i < config->fd_count; i++)
    {
        const StyxLaunchFd *fd = &config->fds[i];
        if (!strcmp(fd->node, fields[1]) || covers(fd->path, fields[0]) || covers(fields[0], fd->path))
        {
            return 0;
        }
    }
    config->fds[config->fd_count++] = (StyxLaunchFd){fields[0], fields[1], access};
    return 1;
}


/* EN/RU: Check final FD mappings after decoding all paths and applying defaults.
 * Проверить выдачи FD после разбора всех путей и применения значений по умолчанию. */
static int compatible_fds(const TerminalLaunchConfig *config)
{
    for (size_t i = 0; i < config->fd_count; i++)
    {
        const StyxLaunchFd *fd = &config->fds[i];
        if (!strcmp(fd->node, config->request) || !strcmp(fd->node, config->paths.receive) ||
            !strcmp(fd->node, config->paths.send) || !strcmp(fd->node, config->code) ||
            (strcmp(config->dependency, "-") && !strcmp(fd->node, config->dependency)))
        {
            return 0;
        }
        for (size_t j = 0; j < config->count; j++)
        {
            const char *path = config->files[j].path;
            if (covers(path, fd->path) || covers(fd->path, path))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: Parse one explicitly inherited descriptor and its nontruncating mode.
 * Разобрать номер передаваемого дескриптора и его режим без усечения. */
static int inherit_fd(TerminalLaunchConfig *config, char **fields)
{
    unsigned long descriptor;
    unsigned access = grant_access(fields[1]);
    if (!access || !number(fields[0], DVM_LAUNCH_FD_COUNT - 1, &descriptor) ||
        config->inherited_fds[descriptor])
    {
        return 0;
    }
    config->inherited_fds[descriptor] = (unsigned char)access;
    return 1;
}


/* EN/RU: Keep each numbered import distinct from code, transport and named file routes.
 * Проверить, что номерные файлы отделены от кода, транспорта и именных выдач. */
static int compatible_inherited(const TerminalLaunchConfig *config)
{
    for (unsigned number = 0; number < DVM_LAUNCH_FD_COUNT; number++)
    {
        if (!config->inherited_fds[number])
        {
            continue;
        }
        char path[DVM_LAUNCH_FD_PATH_SIZE];
        if (!dvm_launch_fd_path(path, number))
        {
            return 0;
        }
        const char *node = path + 1;
        if (!strcmp(node, config->request) || !strcmp(node, config->paths.receive) ||
            !strcmp(node, config->paths.send) || !strcmp(node, config->code) ||
            !strcmp(node, config->dependency))
        {
            return 0;
        }
        for (size_t i = 0; i < config->fd_count; i++)
        {
            if (!strcmp(node, config->fds[i].node))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: See config.h; defaults preserve the original one-shot profile.
 * См. config.h; значения по умолчанию сохраняют прежний профиль. */
int dvm_terminal_launch_config(int argc, char **argv, TerminalLaunchConfig *config)
{
    if (argc < 8 || !argv[3][0] || argv[3][0] == '$' || strlen(argv[3]) >= DVM_NS_PATH_LIMIT ||
        !strcmp(argv[4], ".") || !strcmp(argv[4], "-") || !strcmp(argv[5], ".") ||
        !dvm_terminal_channel_paths_valid(argv[4], argv[5]))
    {
        return 0;
    }
    *config = (TerminalLaunchConfig){0};
    config->fd = argv[2];
    config->label = argv[3];
    config->code = argv[4];
    config->dependency = argv[5];
    config->request = "launch";
    config->paths = (StyxTerminalPaths){"records", "records", NULL};
    config->tag = 17;
    config->lane = 71;
    config->timeout = 5000;
    unsigned seen = 0;
    int at = 6;
    while (at < argc && strcmp(argv[at], "--"))
    {
        unsigned long value;
        if (!strcmp(argv[at], "--request") && at + 2 < argc && !(seen & 1) &&
            dvm_terminal_channel_paths_valid(argv[at + 1], argv[at + 1]) &&
            number(argv[at + 2], 0xffffffffUL, &value) && value)
        {
            config->request = argv[at + 1];
            config->tag = value;
            seen |= 1;
            at += 3;
        }
        else if (!strcmp(argv[at], "--records") && at + 2 < argc && !(seen & 2) &&
            dvm_terminal_channel_paths_valid(argv[at + 1], argv[at + 2]))
        {
            config->paths.receive = argv[at + 1];
            config->paths.send = argv[at + 2];
            seen |= 2;
            at += 3;
        }
        else if (!strcmp(argv[at], "--lane") && at + 1 < argc && !(seen & 4) &&
            number(argv[at + 1], 0xffffffffUL, &value) && value)
        {
            config->lane = value;
            seen |= 4;
            at += 2;
        }
        else if (!strcmp(argv[at], "--timeout-ms") && at + 1 < argc && !(seen & 8) &&
            number(argv[at + 1], 120000, &value) && value)
        {
            config->timeout = (unsigned)value;
            seen |= 8;
            at += 2;
        }
        else if (!strcmp(argv[at], "--grant") && at + 3 < argc && grant(config, argv + at + 1))
        {
            at += 4;
        }
        else if (!strcmp(argv[at], "--fd-grant") && at + 3 < argc && fd_grant(config, argv + at + 1))
        {
            at += 4;
        }
        else if (!strcmp(argv[at], "--inherit-fd") && at + 2 < argc && inherit_fd(config, argv + at + 1))
        {
            at += 3;
        }
        else
        {
            return 0;
        }
    }
    if (at + 1 >= argc || !strcmp(config->request, config->paths.receive))
    {
        return 0;
    }
    config->separator = at;
    if (!config->count)
    {
        char *zero[] = {"/zero", "0", "rw"};
        char *seven[] = {"/seven", "7", "rw"};
        char *nine[] = {"/nine", "9", "r"};
        if (!grant(config, zero) || !grant(config, seven) || !grant(config, nine))
        {
            return 0;
        }
    }
    return compatible_fds(config) && compatible_inherited(config);
}

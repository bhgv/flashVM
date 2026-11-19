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


/* EN/RU: Append one unique path/tag and translate its exact access spelling, 1/0.
 * Добавить уникальный путь и тег с заданными правами, вернуть 1/0. */
static int grant(TerminalLaunchConfig *config, char **fields)
{
    unsigned long tag;
    unsigned access = 0;
    if (!strcmp(fields[2], "r") || !strcmp(fields[2], "rw"))
    {
        access |= FVM_ACCESS_READ;
    }
    if (!strcmp(fields[2], "w") || !strcmp(fields[2], "rw"))
    {
        access |= FVM_ACCESS_WRITE;
    }
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
        return grant(config, zero) && grant(config, seven) && grant(config, nine);
    }
    return 1;
}

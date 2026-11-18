/* Copyright (c) 2026 bhgv
 * EN: Canonical bounded CLI numbers, duplicate checks and preflight before any transport acquisition.
 * RU: Канонические ограниченные числа CLI, проверка повторов и preflight до получения транспорта.
 */
#include "config.h"
#include "flashvm/sordctl.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Singleton command options, ordered exactly like the name table in parse.
 * Однократные параметры, в том же порядке, что таблица имён в parse.
 */
typedef enum CliOption
{
    CLI_PROFILE, /* Explicit none/ordered profile. */
    CLI_FD, /* Delegated carrier descriptor. */
    CLI_CHANNEL, /* P9MX session0/1. */
    CLI_LIMIT, /* Inner record body bound. */
    CLI_TIMEOUT, /* Native RPC/profile deadline. */
    CLI_WINDOW, /* Per-tag future displacement. */
    CLI_LANE, /* Outgoing outer tag. */
    CLI_RECORDS, /* Optional owner receive quota. */
    CLI_ROOT, /* Captured relative root. */
    CLI_RX, /* Receive endpoint. */
    CLI_TX, /* Send endpoint. */
    CLI_OPTIONS /* Exact singleton count. */
} CliOption;


/* EN/RU: Native scalar bound for a checked numeric option; no truncation or signed conversion.
 * Native-граница проверенного числового параметра; без усечения и знакового преобразования.
 */
static unsigned long long maximum(unsigned option)
{
    switch (option)
    {
        case CLI_FD:
            return INT_MAX;
        case CLI_CHANNEL:
            return 1;
        case CLI_LIMIT:
            return 65536 - 64;
        case CLI_TIMEOUT:
            return 120000;
        case CLI_WINDOW:
            return (1ULL << 63) - 1;
        case CLI_LANE:
            return FVM_SRECORD_U32;
        default:
            return (size_t)-1;
    }
}


/* EN/RU: Parse canonical unsigned decimal within supplied maximum; output changes only on success.
 * Разобрать каноническое беззнаковое десятичное число до maximum; output меняется лишь при успехе.
 */
static int number(const char *text, unsigned long long maximum, unsigned long long *output)
{
    unsigned long long value = 0; /* Checked accumulation, no native overflow or truncation. */
    if (!text || !*text || (text[0] == '0' && text[1]))
    {
        return 0;
    }
    for (const char *at = text; *at; at++)
    {
        if (*at < '0' || *at > '9' || (unsigned)(*at - '0') > maximum || value > (maximum - (unsigned)(*at - '0')) / 10)
        {
            return 0;
        }
        value = value * 10 + (unsigned)(*at - '0');
    }
    *output = value;
    return 1;
}


/* EN/RU: Copy one bounded immutable CLI path; canonical validation occurs after complete parse.
 * Скопировать ограниченный неизменяемый путь CLI; каноничность проверяется после полного parse.
 */
static int path(char *output, const char *text)
{
    size_t size = strlen(text); /* Borrowed argv string extent. */
    if (!size || size >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(output, text, size + 1);
    return 1;
}


/* EN/RU: See config.h; parser owns only tags, never descriptors or borrowed argv.
 * См. config.h; parser владеет лишь tags, не descriptors и не argv.
 */
void unibuscli_clear(UnibusCliConfig *config)
{
    if (config)
    {
        free(config->tags);
        memset(config, 0, sizeof(*config));
    }
}


/* EN/RU: See config.h; every unknown, repeated, missing or incompatible option fails before I/O.
 * См. config.h; неизвестный, повторный, отсутствующий или несовместимый параметр отвергается до I/O.
 */
int unibuscli_parse(int argc, char *const *argv, UnibusCliConfig *config, const char **error)
{
    static const char *names[CLI_OPTIONS] = { /* Singleton options; --tag deliberately repeats. */
        "--profile", "--phy-fd", "--channel", "--limit", "--timeout-ms", "--window", "--lane", "--receive-records", "--root", "--rx", "--tx"
    };
    unsigned seen = 0; /* Singleton presence bitset, no semantic inference from defaults. */
    if (!config || !error)
    {
        return 0;
    }
    memset(config, 0, sizeof(*config));
    *error = "invalid arguments";
    if (argc < 1 || !argv)
    {
        return 0;
    }
    config->fd = -1;
    config->limit = 4096;
    config->timeout = 3000;
    config->window = 1024;
    strcpy(config->root, ".");
    strcpy(config->receive, "records");
    strcpy(config->send, "records");
    if ((size_t)argc > (size_t)-1 / sizeof(*config->tags))
    {
        return 0;
    }
    config->tags = calloc((size_t)argc, sizeof(*config->tags));
    if (!config->tags)
    {
        *error = "allocation failed";
        return 0;
    }
    for (int at = 1; at < argc; at++)
    {
        *error = argv[at];
        if (!strcmp(argv[at], "--help"))
        {
            if (config->help)
            {
                return 0;
            }
            config->help = 1;
            continue;
        }
        if (at + 1 >= argc)
        {
            return 0;
        }
        const char *key = argv[at]; /* Current option, never wire content. */
        const char *value = argv[++at]; /* Required immutable argument. */
        unsigned index; /* Singleton discriminator. */
        unsigned long long scalar = 0; /* Exact checked numeric value. */
        if (!strcmp(key, "--tag"))
        {
            if (!number(value, FVM_SRECORD_U32, &scalar))
            {
                return 0;
            }
            for (size_t i = 0; i < config->count; i++)
            {
                if (config->tags[i] == scalar)
                {
                    return 0;
                }
            }
            config->tags[config->count++] = (unsigned long)scalar;
            continue;
        }
        for (index = 0; index < sizeof(names) / sizeof(*names) && strcmp(key, names[index]); index++)
        {
        }
        if (index == sizeof(names) / sizeof(*names) || (seen & (1U << index)))
        {
            return 0;
        }
        seen |= 1U << index;
        if (index == CLI_PROFILE)
        {
            if (strcmp(value, "none") && strcmp(value, "ordered"))
            {
                return 0;
            }
            config->profile = !strcmp(value, "ordered") ? UNIBUS_CLI_ORDERED : UNIBUS_CLI_NONE;
            continue;
        }
        if (index >= CLI_ROOT)
        {
            char *destination = index == CLI_ROOT ? config->root : index == CLI_RX ? config->receive : config->send; /* Exact native path field. */
            if (!path(destination, value))
            {
                return 0;
            }
            continue;
        }
        if (!number(value, maximum(index), &scalar) || (!scalar && index != CLI_CHANNEL && index != CLI_RECORDS) ||
            (index == CLI_FD && scalar < 3))
        {
            return 0;
        }
        if (index == CLI_FD)
        {
            config->fd = (int)scalar;
        }
        else if (index == CLI_CHANNEL)
        {
            config->channel = (unsigned)scalar;
        }
        else if (index == CLI_LIMIT)
        {
            config->limit = (unsigned)scalar;
        }
        else if (index == CLI_TIMEOUT)
        {
            config->timeout = (unsigned)scalar;
        }
        else if (index == CLI_WINDOW)
        {
            config->window = scalar;
        }
        else if (index == CLI_LANE)
        {
            config->lane = (unsigned long)scalar;
        }
        else
        {
            config->records = (size_t)scalar;
        }
    }
    *error = "incompatible profile or incomplete ordered options";
    if (config->help)
    {
        return 1;
    }
    if (config->profile == UNIBUS_CLI_NONE)
    {
        return !(seen & ~1U) && !config->count;
    }
    return config->fd >= 3 && config->lane && config->count &&
        dvm_terminal_channel_paths_valid(config->root, config->root) &&
        dvm_terminal_channel_paths_valid(config->receive, config->send);
}

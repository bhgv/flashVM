/* Copyright (c) 2026 bhgv
 * EN: Validate owner CLI names and pin a catalog of additional Dis executables.
 * RU: Проверка имён CLI владельца и закрепление списка дополнительных образов Dis.
 */


/* EN/RU: See programs.h. / См. programs.h. */
int dvm_terminal_parse_programs(DVM_TerminalPrograms *programs, int argc, char **argv, int *at)
{
    while (*at < argc && !strcmp(argv[*at], "--execute-as"))
    {
        if (argc - *at < 3 || programs->count == STYX_LAUNCH_EXTRA_LIMIT ||
            !styxunibusprogramname(argv[*at + 1]) || !argv[*at + 2][0])
        {
            return 0;
        }
        const char *name = argv[*at + 1];
        for (size_t i = 0; i < programs->count; i++)
        {
            if (!strcmp(name, programs->entries[i].name))
            {
                return 0;
            }
        }
        size_t i = programs->count++;
        programs->entries[i].name = name;
        programs->paths[i] = argv[*at + 2];
        *at += 3;
    }
    return 1;
}


/* EN/RU: See programs.h. / См. programs.h. */
int dvm_terminal_publish_programs(DVM_Namespace *host, DVM_TerminalPrograms *programs)
{
    char catalog[STYX_LAUNCH_EXTRA_LIMIT * (STYX_LAUNCH_NAME_LIMIT + 1)];
    size_t used = 0;
    for (size_t i = 0; i < programs->count; i++)
    {
        StyxUnibusProgram *entry = &programs->entries[i];
        DVM_Provider *file = dvm_terminal_publish_program(host, programs->paths[i]);
        entry->image = file ? snapshot_program(file) : NULL;
        dvm_provider_release(file);
        if (!entry->image)
        {
            fprintf(stderr, "disvm: cannot publish named program %s\n", entry->name);
            return 0;
        }
        size_t size = strlen(entry->name);
        memcpy(catalog + used, entry->name, size);
        used += size;
        catalog[used++] = '\n';
    }
    programs->catalog = dvm_module_adapter(catalog, used);
    return programs->catalog != NULL;
}


/* EN/RU: See programs.h. / См. programs.h. */
void dvm_terminal_release_programs(DVM_TerminalPrograms *programs)
{
    for (size_t i = 0; i < programs->count; i++)
    {
        dvm_provider_release(programs->entries[i].image);
    }
    dvm_provider_release(programs->catalog);
}

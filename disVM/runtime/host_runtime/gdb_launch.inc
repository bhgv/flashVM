/* Copyright (c) 2026 bhgv
 * EN: Optional external native debugger launcher; no GDB dependency without --gdb.
 * RU: Необязательный запуск внешнего native debugger; без --gdb зависимости нет.
 */


/* EN: Re-exec argv[0] under explicitly requested GDB, removing --gdb. Called only
 * before threads. Forward all remaining VM arguments unchanged. Return status2
 * if GDB cannot be launched; external target controls suspension semantics.
 * RU: Перезапустить argv[0] под явно запрошенным GDB, сняв --gdb. Только до threads.
 * Остальные VM arguments передать без изменений. Вернуть2 при отказе запуска;
 * семантика suspension определяется external target. */
static int runtime_launch_gdb(int argc, char **argv)
{
    char **arguments = calloc((size_t)argc + 14, sizeof(char *));
    int index;
    if (!arguments)
    {
        fprintf(stderr, "disvm: cannot allocate GDB launch arguments\n");
        return 2;
    }
    arguments[0] = "gdb";
    arguments[1] = "--nx";
    arguments[2] = "--quiet";
    arguments[3] = "--ex";
    arguments[4] = "set pagination off";
    arguments[5] = "--ex";
    arguments[6] = "set non-stop on";
    arguments[7] = "--args";
    arguments[8] = argv[0];
    for (index = 2; index < argc; index++)
    {
        arguments[index + 7] = argv[index];
    }
    execvp(arguments[0], arguments);
    perror("disvm: explicitly requested GDB could not be launched");
    free(arguments);
    return 2;
}

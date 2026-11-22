# Copyright (c) 2026 bhgv
# EN: Exercise repeatable launches on a mounted launch endpoint.
# RU: Проверить повторные запуски смонтированной программы.

# EN: Launch the bound program and verify its result without remounting.
# RU: Запустить забинженную программу и проверить результат без нового маунта.
run_shared(sys: Sys)
{
    vfs := load Vfsnative Vfsnative->PATH;
    if (sys->bind("/transport/implementation.dis", "/shared-launch.dis", Sys->MREPL) < 0) {
        raise "could not bind the shared Styx program";
    }
    expected := 1;
    if (sys->open("/expect-launch-fail", Sys->OREAD) != nil) {
        expected = -2;
    }
    for (i := 0; i < 2; i++) {
        actual := vfs->run("/shared-launch.dis");
        if (actual != expected)
            raise sys->sprint("launch %d returned %d, expected %d", i, actual, expected);
    }
    if (expected == 1) {
        done := chan of (string, int, int);
        spawn run_parallel(vfs, "wait", 1, done);
        spawn run_parallel(vfs, "release", -2, done);
        for (i := 0; i < 2; i++) {
            (command, actual, wanted) := <-done;
            if (actual != wanted) {
                raise sys->sprint("parallel launch %s returned %d, expected %d", command, actual, wanted);
            }
        }
    }
}

# EN: Each worker uses the same bound path and receives its own completion.
# RU: Каждый поток использует общий bind-путь и получает собственный итог.
run_parallel(vfs: Vfsnative, payload: string, expected: int, done: chan of (string, int, int))
{
    done <-= (payload, vfs->runargs("/shared-launch.dis", array of byte payload), expected);
}


# EN: Run another image from the same mount through its retained bind alias.
# RU: Запустить другой образ с того же маунта через удержанный bind-путь.
run_named(sys: Sys)
{
    if (sys->open("/transport/.programs", Sys->OREAD) == nil) {
        return;
    }
    if (sys->bind("/transport/programs/second.dis", "/named-launch.dis", Sys->MREPL) < 0) {
        raise "could not bind the second Styx program";
    }
    vfs := load Vfsnative Vfsnative->PATH;
    if (vfs->run("/named-launch.dis") != -2 ||
        vfs->runargs("/named-launch.dis", array of byte "secondary") != 1 ||
        vfs->runargs("/transport/programs/second.dis", array of byte "secondary") != 1) {
        raise "named Styx launch selected the wrong image or result";
    }
    sys->print("PASS named Styx executable on the same mount\n");
}

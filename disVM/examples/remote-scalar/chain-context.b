# Copyright (c) 2026 bhgv
# EN: Resolve the local numeric name of the active caller grant in the fixture.
# RU: Найти локальное числовое имя активного контекста в примере.


# EN: Read the trusted scoped PID file and form its canonical path.
# RU: Прочитать PID из доверенного контекста и собрать канонический путь.
chain_context_path(sys: Sys): string
{
    pid := sys->open("/.prog/.self/.pid", Sys->OREAD);
    data := array[16] of byte;
    if (pid == nil) {
        raise "caller .pid unavailable";
    }
    count := sys->read(pid, data, len data);
    if (count < 2 || data[count-1] != byte '\n') {
        raise "invalid caller .pid";
    }
    spelling := string data[:count-1];
    number := int spelling;
    if (number <= 0 || string number != spelling) {
        raise "noncanonical caller .pid";
    }
    return "/.prog/" + spelling;
}

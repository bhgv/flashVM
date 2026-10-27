implement Filesystem; # Name this executable module. / Имя исполняемого модуля.

include "sys.m"; # Import the matching SDK declaration. / Объявления выбранного ABI.
include "readdir.m"; # Import the matching SDK declaration. / Объявления выбранного ABI.

Filesystem: module # Prepare this local test value or declaration. / Локальное значение или объявление.
{ # Begin this module or function scope. / Начало области.
    init: fn(); # Standalone matched-ABI regression entry.
}; # Finish this scope. / Конец области.

sys: Sys; # Real native services behind the platform filesystem ports.
root: con FIXTURE_ROOT; # The test driver substitutes one private UTF-8 fixture path.
failed: int; # Sticky oracle failure, printed before returning to the host.


# Keep every predicate active; actual native failures must not look like success.
# condition is a scalar and message is borrowed. No owner escapes this function.
check(condition: int, message: string) # Check the external oracle; preserve a visible failure. / Проверка результата.
{ # Begin this module or function scope. / Начало области.
    if(!condition) { # Branch on the actual result. / Проверка условия.
        sys->print("FAIL filesystem: %s\n", message); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
        failed = 1; # Prepare this local test value or declaration. / Локальное значение или объявление.
    } # Finish this scope. / Конец области.
} # Finish this scope. / Конец области.


# Exercise actual Sys and Readdir modules. Read-only metadata must preserve file
# positions, binary data and owner lifetimes, including collection every opcode.
init() # Prepare this local test value or declaration. / Локальное значение или объявление.
{ # Begin this module or function scope. / Начало области.
    sys = load Sys Sys->PATH; # Prepare this local test value or declaration. / Локальное значение или объявление.
    (status, info) := sys->stat(root+"/данные😀.bin"); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(status == 0 && info.name == "данные😀.bin" && info.length == big 4, "Unicode stat"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    check((info.mode & Sys->DMDIR) == 0 && info.qid.qtype == Sys->QTFILE, "file kind"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (large_status, large) := sys->stat(root+"/large.bin"); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(large_status == 0 && large.length == big 4294967303, "full 64-bit sparse length"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (directory_status, directory) := sys->stat(root); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(directory_status == 0 && (directory.mode & Sys->DMDIR) != 0 && directory.qid.qtype == Sys->QTDIR, "directory kind"); # Check the external oracle; preserve a visible failure. / Проверка результата.

    fd := sys->open(root+"/данные😀.bin", Sys->OREAD); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(fd != nil, "open file"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    bytes := array[1] of byte; # Prepare this local test value or declaration. / Локальное значение или объявление.
    check(sys->read(fd, bytes, 1) == 1 && int bytes[0] == 65, "initial byte"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (open_status, opened) := sys->fstat(fd); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(open_status == 0 && opened.length == big 4 && opened.qid.path == info.qid.path, "open identity"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    check(sys->read(fd, bytes, 1) == 1 && int bytes[0] == 0, "fstat preserves cursor and NUL"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (n, wrong) := sys->dirread(fd); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(n == -1 && wrong == nil, "file is not a directory"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    fd = nil; # The last reference closes the platform file handle.

    fd = sys->open(root, Sys->OREAD); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    (open_status, opened) = sys->fstat(fd); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(open_status == 0 && opened.qid.path == directory.qid.path, "directory descriptor identity"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    total := batches := 0; # Prepare this local test value or declaration. / Локальное значение или объявление.
    found := 0; # Prepare this local test value or declaration. / Локальное значение или объявление.
    for(;;) { # Traverse the bounded fixture or repeat owner retirement. / Цикл проверки.
        (count, entries) := sys->dirread(fd); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
        check(count >= 0 && count == len entries && count <= 32, "bounded directory batch"); # Check the external oracle; preserve a visible failure. / Проверка результата.
        if(count <= 0) { # Branch on the actual result. / Проверка условия.
            break; # Leave the loop at EOF or error. / Завершить обход.
        } # Finish this scope. / Конец области.
        batches++; # Accumulate the observed fixture count. / Счётчик фактических элементов.
        total += count; # Accumulate the observed fixture count. / Счётчик фактических элементов.
        for(index := 0; index < count; index++) { # Traverse the bounded fixture or repeat owner retirement. / Цикл проверки.
            check(entries[index].name != "." && entries[index].name != "..", "dot entries excluded"); # Check the external oracle; preserve a visible failure. / Проверка результата.
            if(entries[index].name == "данные😀.bin") { # Branch on the actual result. / Проверка условия.
                check(entries[index].length == big 4, "entry metadata"); # Check the external oracle; preserve a visible failure. / Проверка результата.
                found++; # Accumulate the observed fixture count. / Счётчик фактических элементов.
            } # Finish this scope. / Конец области.
        } # Finish this scope. / Конец области.
    } # Finish this scope. / Конец области.
    check(total == 73 && batches == 3 && found == 1, "complete multi-batch enumeration"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (again, eof) := sys->dirread(fd); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(again == 0 && eof == nil, "stable directory EOF"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    check(sys->read(fd, bytes, 1) == -1 && sys->seek(fd, big 0, Sys->SEEKSTART) == big -1, "directory stream type"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    fd = nil; # Release this VM reference; final ownership closes its native resource. / Освобождение ссылки.

    # Readdir is the original ordinary Limbo implementation, now using real I/O.
    readdir := load Readdir "readdir.dis"; # Prepare this local test value or declaration. / Локальное значение или объявление.
    (sorted, count) := readdir->init(root, Readdir->NAME); # Run the original library using hosted Sys directory services. / Вызов Readdir.
    check(count == 73 && len sorted == count, "real Readdir init"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    for(index := 1; index < count; index++) { # Traverse the bounded fixture or repeat owner retirement. / Цикл проверки.
        check(sorted[index-1].name <= sorted[index].name, "Readdir name ordering"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    } # Finish this scope. / Конец области.
    sorted = nil; # Release this VM reference; final ownership closes its native resource. / Освобождение ссылки.

    # More opens than the runtime slot quota must succeed when owners retire.
    for(index = 0; index < 80; index++) { # Traverse the bounded fixture or repeat owner retirement. / Цикл проверки.
        fd = sys->open(root, Sys->OREAD); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
        check(fd != nil, "directory finalizer quota reuse"); # Check the external oracle; preserve a visible failure. / Проверка результата.
        fd = nil; # Release this VM reference; final ownership closes its native resource. / Освобождение ссылки.
        fd = sys->open(root+"/данные😀.bin", Sys->OREAD); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
        check(fd != nil, "file finalizer quota reuse"); # Check the external oracle; preserve a visible failure. / Проверка результата.
        fd = nil; # Release this VM reference; final ownership closes its native resource. / Освобождение ссылки.
    } # Finish this scope. / Конец области.
    (status, info) = sys->stat(root+"/missing"); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(status == -1 && info.name == nil && info.uid == nil, "missing path clears previous roots"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    nul := "bad bad"; # Prepare this local test value or declaration. / Локальное значение или объявление.
    nul[3] = 0; # Prepare this local test value or declaration. / Локальное значение или объявление.
    (status, info) = sys->stat(nul); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(status == -1 && info.name == nil, "embedded NUL rejected"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    (status, info) = sys->stat("content://documents/not-a-native-path"); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    check(status == -1, "provider URI is not a filesystem path"); # Check the external oracle; preserve a visible failure. / Проверка результата.
    if(!failed) { # Branch on the actual result. / Проверка условия.
        sys->print("PASS Sys filesystem: metadata, batches, Readdir, quota and errors\n"); # Call the real Sys service through the selected OS port. / Настоящий вызов Sys.
    } # Finish this scope. / Конец области.
} # Finish this scope. / Конец области.

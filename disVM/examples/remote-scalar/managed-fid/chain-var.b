# Copyright (c) 2026 bhgv
# EN: Read and update a live numbered cell through the current caller's Styx view.
# RU: Прочитать и обновить нативную ячейку через встречную VFS вызывающего.


# EN: Verify the exported int32 bytes and optionally write a new small value.
# RU: Проверить байты выданного int32 и при необходимости записать новое значение.
chain_var(sys: Sys, expected, replacement: int)
{
    check_cell(sys, "chain-var", expected, replacement);
}


# EN: Confirm the inherited read-only rule and absence of an unpublished host file.
# RU: Проверить унаследованное право чтения и отсутствие неопубликованного файла.
chain_readonly_var(sys: Sys)
{
    check_cell(sys, "chain-readonly", 9, -1);
    if (sys->open("/.prog/.self/.var/chain-readonly", Sys->OWRITE) != nil ||
        sys->open("/.prog/.self/.var/unpublished", Sys->OREAD) != nil) {
        raise "nested variable grant widened access";
    }
}


# EN: Read a cell under the current caller's variable grant, optionally updating it.
# RU: Прочитать и при необходимости обновить ячейку в выдаче вызывающего.
check_cell(sys: Sys, name: string, expected, replacement: int)
{
    mode := Sys->OREAD;
    if (replacement >= 0) {
        mode = Sys->ORDWR;
    }
    fd := sys->open("/.prog/.self/.var/"+name, mode);
    data := array[4] of byte;
    if (fd == nil || sys->readn(fd, data, len data) != len data ||
        data[0] != byte 0 || data[1] != byte 0 || data[2] != byte 0 || data[3] != byte expected) {
        raise sys->sprint("chain .var %s read expected %d: %r", name, expected);
    }
    if (replacement >= 0) {
        data[3] = byte replacement;
        if (sys->seek(fd, big 0, Sys->SEEKSTART) != big 0 ||
            sys->write(fd, data, len data) != len data) {
            raise sys->sprint("chain .var %s write expected %d: %r", name, replacement);
        }
    }
}

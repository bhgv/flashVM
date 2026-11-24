# Copyright (c) 2026 bhgv
# EN: Receive one released stroke as ordered, bounded pieces of copied data.
# RU: Принять завершённый штрих упорядоченными фрагментами скопированных данных.


# EN: Assembly lives only in the desktop's input turn, for one view ticket.
# RU: Буфер принадлежит текущему циклу ввода рабочего стола и одному билету.
PairedInk: adt
{
    slot: int; # Stable vector pane 1..8.
    revision: int; # Source revision from its vector declaration.
    x: int; # Window pixel of the initial press.
    y: int; # Window pixel of the initial press.
    width: int; # Number of bytes per sample, including optional elapsed time.
    used: int; # Number of bytes already accepted.
    points: array of byte; # Immutable packed points when complete.
};


# EN: Append a fragment for the current ticket. Payload is eight hex digits
# each for pane, revision, sample offset and total. Optional 't' adds six
# elapsed-time digits to each five-digit point. Return nil on malformed/stale
# fragments. The caller owns
# the assembly and discards it on unrelated input; no pane receives partial data.
# RU: Добавить фрагмент текущего билета. Панель, ревизия, смещение и число
# точек задаются по восемь шестнадцатеричных цифр. Маркер 't' добавляет шесть
# цифр времени к пяти цифрам каждой точки. Владелец сбрасывает буфер при другом
# вводе. В панель поступает только полный штрих.
appendink(codec: Uibridge, event: ref Uibridge->Message, ticket: int,
          stroke: ref PairedInk): ref PairedInk
{
    bytes := array of byte event.text;
    start := 32;
    width := 5;
    if (event.flags != ticket || len bytes <= start) {
        return nil;
    }
    if (int bytes[start] == 't') {
        start++;
        width = 11;
    }
    payload := len bytes - start;
    if (payload <= 0 || payload % width != 0) {
        return nil;
    }
    (slotok, slot) := codec->parseword(bytes, 0);
    (revisionok, revision) := codec->parseword(bytes, 8);
    (offsetok, offset) := codec->parseword(bytes, 16);
    (countok, count) := codec->parseword(bytes, 24);
    if (!slotok || !revisionok || !offsetok || !countok || slot < 1 || slot > Idedesktop->MaxWindows ||
        revision < 0 || count < 2 || count > Idevector->MaxNodes ||
        offset < 0 || offset > count || payload > width * (count - offset)) {
        return nil;
    }
    if (offset == 0) {
        stroke = ref PairedInk(slot, revision, event.first, event.second, width, 0, array[width * count] of byte);
    }
    if (stroke == nil || slot != stroke.slot || revision != stroke.revision ||
        event.first != stroke.x || event.second != stroke.y ||
        width != stroke.width || width * count != len stroke.points || width * offset != stroke.used) {
        return nil;
    }
    stroke.points[stroke.used:] = bytes[start:];
    stroke.used += payload;
    return stroke;
}

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
    used: int; # Number of bytes already accepted.
    points: array of byte; # Immutable packed points when complete.
};


# EN: Append a fragment for the current ticket. Payload is eight hex digits
# each for pane, revision, sample offset and total, then five hex digits per
# point (x + 1001*y). Return nil for malformed/stale fragments. The caller owns
# the assembly and discards it on unrelated input; no pane receives partial data.
# RU: Добавить фрагмент текущего билета. Панель, ревизия, смещение и число
# точек задаются по восемь шестнадцатеричных цифр. Каждая точка x + 1001*y
# занимает пять цифр. Ошибка даёт nil. Владелец сбрасывает буфер при другом
# вводе. В панель поступает только полный штрих.
appendink(codec: Uibridge, event: ref Uibridge->Message, ticket: int,
          stroke: ref PairedInk): ref PairedInk
{
    bytes := array of byte event.text;
    payload := len bytes - 32;
    if (event.flags != ticket || payload <= 0 || payload % 5 != 0) {
        return nil;
    }
    (slotok, slot) := codec->parseword(bytes, 0);
    (revisionok, revision) := codec->parseword(bytes, 8);
    (offsetok, offset) := codec->parseword(bytes, 16);
    (countok, count) := codec->parseword(bytes, 24);
    if (!slotok || !revisionok || !offsetok || !countok || slot < 1 || slot > Idedesktop->MaxWindows ||
        revision < 0 || count < 2 || count > Idevector->MaxNodes ||
        offset < 0 || offset > count || payload > 5 * (count - offset)) {
        return nil;
    }
    if (offset == 0) {
        stroke = ref PairedInk(slot, revision, event.first, event.second, 0, array[5 * count] of byte);
    }
    if (stroke == nil || slot != stroke.slot || revision != stroke.revision ||
        event.first != stroke.x || event.second != stroke.y ||
        5 * count != len stroke.points || 5 * offset != stroke.used) {
        return nil;
    }
    stroke.points[stroke.used:] = bytes[32:];
    stroke.used += payload;
    return stroke;
}

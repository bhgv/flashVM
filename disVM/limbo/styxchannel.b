# Copyright (c) 2026 bhgv
# EN: Translate typed Limbo messages to the mounted owner's native RPC calls.
# RU: Перевод типизированных сообщений Limbo в нативные RPC-вызовы владельца маунта.
implement Styxchannel;
include "sys.m";
include "math.m";
include "unibus.m";
include "styxintref.m";
include "styxchannel.m";

unibus: Unibus; # Mounted-session host functions, loaded on init.
math: Math; # Exact binary64 conversion, loaded on init.
refs: Styxintref; # Typed issuer lease adapter.
Cell: import refs;
Int, Real, Bool, Text, Null, Ref: con iota + 1; # Local builtin framing, not negotiated wire tags.
Wordbytes: con 8; # Width of a local numerical message.


# EN/RU: See styxchannel.m. / См. styxchannel.m.
init()
{
    unibus = load Unibus Unibus->PATH;
    math = load Math Math->PATH;
    refs = load Styxintref Styxintref->PATH;
    if (unibus == nil || math == nil || refs == nil) {
        raise "cannot load StyxRPC channel modules";
    }
    refs->init();
}


# EN/RU: See styxchannel.m. / См. styxchannel.m.
newchan(address, root: string): ref Port
{
    if (unibus == nil || root == nil || root[0] != '/') {
        raise "invalid StyxRPC mount";
    }
    handle := unibus->rpcopen(address);
    if (handle <= 0) {
        raise "cannot open StyxRPC exchange";
    }
    return ref Port(handle, root);
}


# EN: Pack one signed integer or real bit pattern into the local builtin frame.
# RU: Упаковать целое или биты real в локальный кадр builtin.
pack(kind: int, bits: big): array of byte
{
    frame := array[Wordbytes+1] of byte;
    frame[0] = byte kind;
    for (i := Wordbytes; i > 0; --i) {
        frame[i] = byte bits;
        bits >>= 8;
    }
    return frame;
}


# EN: Recover one exact two's-complement word from a local builtin frame.
# RU: Восстановить точное слово в дополнительном коде из локального кадра builtin.
word(frame: array of byte): big
{
    if (len frame != Wordbytes+1) {
        raise "invalid StyxRPC numeric frame";
    }
    bits := big 0;
    for (i := 1; i < len frame; ++i) {
        bits = (bits << 8) | big frame[i];
    }
    return bits;
}


# EN: Copy counted bytes into a builtin envelope that begins with one type byte.
# RU: Скопировать байты с длиной в локальный кадр с первым байтом типа.
packet(kind: int, payload: array of byte): array of byte
{
    frame := array[len payload+1] of byte;
    frame[0] = byte kind;
    if (len payload) {
        frame[1:] = payload;
    }
    return frame;
}


# EN: Select a canonical local envelope for message on the port's issuer mount.
# RU: Выбрать канонический локальный кадр для message в маунте владельца порта.
encode(port: ref Port, message: ref Message): array of byte
{
    pick item := message {
    Integer =>
        return pack(Int, big item.value);
    Floating =>
        return pack(Real, math->realbits64(item.value));
    Boolean =>
        if (item.value != 0 && item.value != 1) {
            raise "invalid StyxRPC Boolean";
        }
        return array[] of {byte Bool, byte item.value};
    Text =>
        return packet(Text, array of byte item.value);
    Null =>
        return array[] of {byte Null};
    Variable =>
        if (item.cell == nil || item.cell.root != port.root) {
            raise "StyxRPC reference issuer mismatch";
        }
        return packet(Ref, item.cell.payload());
    }
    raise "invalid StyxRPC message";
}


# EN/RU: See styxchannel.m. / См. styxchannel.m.
Port.send(port: self ref Port, message: ref Message): int
{
    if (port.handle <= 0 || message == nil) {
        raise "invalid StyxRPC send";
    }
    outcome := unibus->rpcsend(port.handle, encode(port, message));
    if (outcome < 0) {
        raise "StyxRPC send failed";
    }
    return outcome == 0;
}


# EN/RU: See styxchannel.m. / См. styxchannel.m.
Port.recv(port: self ref Port): ref Message
{
    if (port.handle <= 0) {
        raise "closed StyxRPC exchange";
    }
    frame := unibus->rpcrecv(port.handle);
    if (frame == nil) {
        return nil;
    }
    case int frame[0] {
    Int =>
        return ref Message.Integer(int word(frame));
    Real =>
        return ref Message.Floating(math->bits64real(word(frame)));
    Bool =>
        return ref Message.Boolean(int frame[1]);
    Text =>
        return ref Message.Text(string frame[1:]);
    Null =>
        return ref Message.Null(0);
    Ref =>
        return ref Message.Variable(refs->accept(port.root, frame[1:]));
    }
    raise "invalid StyxRPC message kind";
}


# EN/RU: See styxchannel.m. / См. styxchannel.m.
Port.close(port: self ref Port)
{
    if (port.handle > 0) {
        if (unibus->rpcclose(port.handle) != 0) {
            raise "StyxRPC close failed";
        }
        port.handle = 0;
    }
}

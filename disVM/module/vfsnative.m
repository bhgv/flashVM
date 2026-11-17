# Copyright (c) 2026 bhgv
# EN: Scalar native bridge for caller-namespace layer control; prefer Vfsctl wrapper.
# RU: Scalar native bridge управления слоями своего namespace; удобный wrapper Vfsctl.
Vfsnative: module
{
    PATH: con "$Vfsnative"; # Hosted builtin, no foreign namespace/PID input.

    # EN/RU: status,revision,head,id,count,position,flags,kind,metadata_status,mode,directory,access.
    # Читать copied card; revision=-1 captures current, stale=-2; no pointer results.
    layer: fn(path: string, revision: big, index: int): (int,big,big,big,int,int,int,int,int,int,int,int);

    # EN/RU: Read mount0/relative1 or launch role2/system3/export4/session5/nickname6, one UTF-8 byte.
    # Читать byte: status1/byte,0/end,-2/stale,-1/error. Never host absolute source paths.
    text: fn(path: string, revision: big, index, part, offset: int): (int,int);

    # EN/RU: status, source session/generation, native capability bits; zero for native anchor.
    # Copied source state, не permission grant и не global registry.
    state: fn(path: string, revision: big, index: int): (int,big,big,int);

    # EN/RU: Bind at a signed insertion slot; flags16=MINS, index0 otherwise.
    # Bind в signed slot; return0 success,-1 error, same source/target validation path.
    bind: fn(source, target: string, flags, index: int): int;

    # EN/RU: Move exact observed ID at revision; inherited child path gets its own stack.
    # Переставить ID по revision; return0 success,-2/stale,-1/error; no effect replay.
    move: fn(path: string, revision, id: big, index: int): int;
};

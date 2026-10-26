implement Brokerdiag;
include "brokerdiag.m";


# Decoder state borrows the input array only during decode; no state escapes on failure.
Decoder: adt
{
    frame: array of byte; # Borrowed counted ASCII frame, retained by this temporary Dis reference.
    at: int; # Next wire byte offset.
    ok: int; # Sticky canonical-format failure flag.
};


# Read a bounded canonical hex field. d is temporary state; width is 2, 8 or 16.
# Return an owned string, preserving uint64 identity on classic WORD32 and NC64 alike.
word(d: ref Decoder, width: int): string
{
    if(!d.ok || width > len d.frame - d.at){
        d.ok = 0;
        return "";
    }
    result := string d.frame[d.at:d.at + width];
    for(i := 0; i < width; i++){
        c := int d.frame[d.at + i];
        if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))){
            d.ok = 0;
        }
    }
    d.at += width;
    return result;
}


# Convert an already checked short hex field; callers bound widths before allocation/loops.
number(value: string): int
{
    result := 0;
    for(i := 0; i < len value; i++){
        c := value[i];
        digit := c - '0';
        if(c >= 'a'){
            digit = c - 'a' + 10;
        }
        result = (result << 4) | digit;
    }
    return result;
}


# Decode one bounded text field, verifying UTF-8 by exact byte round trip before publication.
# limit includes the required native terminator but no terminator is carried on the wire.
text(d: ref Decoder, limit: int): string
{
    sizeword := word(d, 8);
    if(!d.ok || sizeword > "00002000"){
        d.ok = 0;
        return "";
    }
    size := number(sizeword);
    if(size >= limit || size > (len d.frame - d.at) / 2){
        d.ok = 0;
        return "";
    }
    bytes := array[size] of byte;
    for(i := 0; i < size; i++){
        bytes[i] = byte number(word(d, 2));
        if(bytes[i] == byte 0){
            d.ok = 0;
        }
    }
    result := string bytes;
    check := array of byte result;
    if(len check != size){
        d.ok = 0;
    }else{
        for(i = 0; i < size; i++){
            if(check[i] != bytes[i]){
                d.ok = 0;
            }
        }
    }
    return result;
}


# Read copied source fields from d. The returned ADT contains no foreign heap references.
source(d: ref Decoder): ref Site
{
    site := ref Site("", "", "", "", "", "", "", "");
    site.build = text(d, 65);
    site.path = text(d, 256);
    site.moduleName = text(d, 128);
    site.line = word(d, 8);
    site.column = word(d, 8);
    site.offset = word(d, 16);
    site.length = word(d, 8);
    site.binding = word(d, 8);
    return site;
}


# Validate present/absent origins and unsigned byte extents without narrowing offsets to int.
validsite(site: ref Site, absent: int): int
{
    if(absent){
        return site.build == "" && site.path == "" && site.moduleName == "" && site.line == "00000000" &&
               site.column == "00000000" && site.offset == "0000000000000000" &&
               site.length == "00000000" && site.binding == "00000000";
    }
    carry := 0;
    extent := "00000000" + site.length;
    for(i := 15; i >= 0; i--){
        left := number(site.offset[i:i + 1]);
        right := number(extent[i:i + 1]);
        carry = left + right + carry >= 16;
    }
    return site.build != "" && site.path != "" && site.line != "00000000" &&
           site.column != "00000000" && site.length != "00000000" && !carry;
}


# Decode one complete frame; malformed input yields no partial Event or executable owner call.
decode(frame: array of byte): (int, ref Event)
{
    if(len frame < 3 || len frame > MaxFrame || string frame[0:3] != "BD1"){
        return (0, nil);
    }
    d := ref Decoder(frame, 3, 1);
    event := ref Event(0, "", "", "", "", "", "", "", "", 0, 0, "", "", "", 0, "", "", "", "", "", nil, nil, nil);
    kindword := word(d, 8);
    if(kindword > "0000000f"){ d.ok = 0; }
    event.kind = number(kindword);
    event.at = word(d, 16);
    event.channel = word(d, 8);
    event.generation = word(d, 8);
    event.requestId = word(d, 16);
    event.parentChannel = word(d, 8);
    event.parentGeneration = word(d, 8);
    event.parentRequest = word(d, 16);
    event.objectId = word(d, 16);
    phaseword := word(d, 8);
    if(phaseword > "0000000f"){ d.ok = 0; }
    event.phase = number(phaseword);
    flagsword := word(d, 8);
    if(flagsword > "0000000f"){ d.ok = 0; }
    event.flags = number(flagsword);
    event.started = word(d, 16);
    event.updated = word(d, 16);
    event.progress = word(d, 16);
    transportword := word(d, 8);
    if(transportword > "0000000f"){ d.ok = 0; }
    event.transport = number(transportword);
    event.logical = text(d, 64);
    event.localAddress = text(d, 128);
    event.peer = text(d, 128);
    event.operation = text(d, 64);
    event.detail = text(d, 128);
    event.origin = source(d);
    event.creation = source(d);
    event.connection = source(d);
    if(!d.ok || d.at != len frame || event.kind < 1 || event.kind > 5 || event.phase > 8 ||
       event.flags > 7 || event.transport < 1 || event.transport > 3 || event.channel == "00000000" ||
       event.generation == "00000000" || event.logical == "" || event.peer == "" ||
       (event.requestId != "0000000000000000" && event.operation == "") ||
       event.updated > event.at || event.started > event.updated){
        return (0, nil);
    }
    if(!validsite(event.origin, event.requestId == "0000000000000000") ||
       !validsite(event.creation, 0) || !validsite(event.connection, 0)){
        return (0, nil);
    }
    return (1, event);
}

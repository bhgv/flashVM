# Common UI envelopes; transport endpoints are ordinary chan of array of byte.
Uibridge: module
{
    PATH: con "uibridge.dis";  # Relative module resource selected by the embedding namespace.
    MaxText: con 1024;         # Maximum encoded payload bytes, not Unicode character count.
    Header: con 53;            # ASCII prefix and six fixed-width hexadecimal fields.
    Request,                   # Service request with a nonzero correlation ID.
    Reply,                     # Reply with the matching correlation ID.
    Pointer,                   # Mouse position in twips; code is button, flags is phase.
    Key,                       # Virtual key code; flags is pressed, first is ASCII.
    Text,                      # Committed text from FlashVM host input.
    Focus,                     # Focus transition from FlashVM.
    View: con iota + 1;        # Dis-owned snapshot/selection presentation; id is zero.


    # Copied application values; no pointer, GC handle or shared heap representation.
    Message: adt
    {
        kind: int;     # One operation class above.
        id: int;       # Correlation ID bits; zero only for input events.
        code: int;     # Class-specific unsigned operation/status bits.
        flags: int;    # Class-specific unsigned flags.
        first: int;    # Signed first argument or pointer X in twips.
        second: int;   # Signed second argument or pointer Y in twips.
        text: string;  # UTF-8 payload on the wire, including any embedded zero.
    };


    # Validate message and copy it to an owned wire byte array. Returns (1, frame),
    # or (0, nil) for invalid kind/ID/byte extent. Allocation errors are Dis exceptions.
    encode: fn(message: ref Message): (int, array of byte);


    # Validate and decode a borrowed frame. Returns (1, owned message) or (0, nil).
    # Neither function performs channel operations or imports a native OS module.
    decode: fn(frame: array of byte): (int, ref Message);


    # Format an int bit pattern as eight canonical hexadecimal ASCII characters.
    # value is copied; returns a new string without I/O or native dependencies.
    hexword: fn(value: int): string;


    # Parse a bounded eight-byte hexadecimal field. frame is borrowed; offset is a byte index.
    # Returns (1, int bits) or (0, 0) for malformed/out-of-range input.
    parseword: fn(frame: array of byte, offset: int): (int, int);
};

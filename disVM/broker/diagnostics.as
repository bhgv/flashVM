// Decode one bounded BD1 diagnostic into an object owned by the receiving AS VM.
// frame is a copied ASCII string, never a foreign object or ordinary RPC result.
// Returns undefined on malformed input; 64-bit fields remain exact hexadecimal strings.
function decodeBrokerDiagnostic(frame)
{
    if (typeof frame != "string" || frame.length < 3 || frame.length > 8192 || frame.substr(0, 3) != "BD1")
    {
        return undefined;
    }
    var at = 3; // Cursor in the ASCII wire frame; byte and AS character offsets are identical here.
    var ok = true; // Sticky failure state; no partial event is published.


    // Read count lowercase hex characters; preserve wide IDs without converting through Number.
    function word(count)
    {
        if (!ok || at + count > frame.length)
        {
            ok = false;
            return "";
        }
        var value = frame.substr(at, count); // Exact canonical integer spelling.
        at += count;
        for (var i = 0; i < count; i++)
        {
            var c = value.charCodeAt(i); // Current ASCII digit.
            if (!((c >= 48 && c <= 57) || (c >= 97 && c <= 102)))
            {
                ok = false;
            }
        }
        return value;
    }


    // Decode bounded hex UTF-8, rejecting NUL, overlong sequences and invalid scalar values.
    function text(limit)
    {
        var size = parseInt(word(8), 16); // The length is validated before any allocation loop.
        if (!ok || size >= limit || size * 2 > frame.length - at)
        {
            ok = false;
            return "";
        }
        var bytes = new Array(); // Local byte values, never a cross-heap pointer.
        for (var i = 0; i < size; i++)
        {
            bytes[i] = parseInt(word(2), 16);
        }
        var result = ""; // Newly owned AS UTF-16 text.
        var pos = 0; // Next UTF-8 byte.
        while (pos < size && ok)
        {
            var first = bytes[pos++]; // Leading byte or one ASCII scalar.
            var value = first; // Accumulated Unicode scalar.
            var count = 0; // Continuation byte count.
            var minimum = 0; // Smallest scalar admitted by the selected sequence width.
            if (first == 0)
            {
                ok = false;
            }
            else if (first < 128)
            {
                count = 0;
            }
            else if (first >= 194 && first <= 223)
            {
                value = first & 31;
                count = 1;
                minimum = 128;
            }
            else if (first >= 224 && first <= 239)
            {
                value = first & 15;
                count = 2;
                minimum = 2048;
            }
            else if (first >= 240 && first <= 244)
            {
                value = first & 7;
                count = 3;
                minimum = 65536;
            }
            else
            {
                ok = false;
            }
            if (pos + count > size)
            {
                ok = false;
            }
            for (var j = 0; j < count && ok; j++)
            {
                var next = bytes[pos++]; // A validated continuation contributes six bits.
                if ((next & 192) != 128)
                {
                    ok = false;
                }
                value = (value << 6) | (next & 63);
            }
            if (value < minimum || value > 1114111 || (value >= 55296 && value <= 57343))
            {
                ok = false;
            }
            if (value <= 65535)
            {
                result += String.fromCharCode(value);
            }
            else
            {
                value -= 65536;
                result += String.fromCharCode(55296 + (value >> 10), 56320 + (value & 1023));
            }
        }
        return result;
    }


    // Read an owned source descriptor; coordinates stay exact hex strings until display conversion.
    function source()
    {
        var s = new Object(); // One local source site, not an executable accessor.
        s.build = text(65);
        s.path = text(256);
        s.moduleName = text(128);
        s.line = word(8);
        s.column = word(8);
        s.offset = word(16);
        s.length = word(8);
        s.binding = word(8);
        return s;
    }


    // Check source extents without rounding uint64 offsets through floating-point Number.
    function validSource(s, absent)
    {
        if (absent)
        {
            return s.build == "" && s.path == "" && s.moduleName == "" && s.line == "00000000"
                && s.column == "00000000" && s.offset == "0000000000000000"
                && s.length == "00000000" && s.binding == "00000000";
        }
        var carry = 0; // Carry above bit 63 means an overflowing source extent.
        var length = "00000000" + s.length; // Extend uint32 byte length to sixteen digits.
        for (var i = 15; i >= 0; i--)
        {
            var sum = parseInt(s.offset.charAt(i), 16) + parseInt(length.charAt(i), 16) + carry;
            carry = sum >= 16 ? 1 : 0;
        }
        return s.build != "" && s.path != "" && s.line != "00000000" && s.column != "00000000"
            && s.length != "00000000" && carry == 0;
    }

    var e = new Object(); // Publish only after every field and the complete frame are validated.
    e.kind = parseInt(word(8), 16);
    e.at = word(16);
    e.channel = word(8);
    e.generation = word(8);
    e.requestId = word(16);
    e.parentChannel = word(8);
    e.parentGeneration = word(8);
    e.parentRequest = word(16);
    e.objectId = word(16);
    e.phase = parseInt(word(8), 16);
    e.flags = parseInt(word(8), 16);
    e.started = word(16);
    e.updated = word(16);
    e.progress = word(16);
    e.transport = parseInt(word(8), 16);
    e.logical = text(64);
    e.localAddress = text(128);
    e.peer = text(128);
    e.operation = text(64);
    e.detail = text(128);
    e.origin = source();
    e.creation = source();
    e.connection = source();
    if (!ok || at != frame.length || e.kind < 1 || e.kind > 5 || e.phase < 0 || e.phase > 8
        || e.flags > 7 || e.transport < 1 || e.transport > 3 || e.channel == "00000000"
        || e.generation == "00000000" || e.logical == "" || e.peer == ""
        || (e.requestId != "0000000000000000" && e.operation == "")
        || e.updated > e.at || e.started > e.updated || !validSource(e.origin, e.requestId == "0000000000000000")
        || !validSource(e.creation, false) || !validSource(e.connection, false))
    {
        return undefined;
    }
    return e;
}


// Wrap the host-provided global diagnostic channel. rx is a rooted FlashVM channel wrapper.
// The host forwards only deviations from dvm_broker_watch(broker, 0, 0, ...).
// Polling is nonblocking on the UI owner; absence of a record is not a success notification.
function makeBrokerDiagnostics(rx)
{
    var api = new Object(); // Helper and received values belong solely to this AS context.
    api.tryReceive = function()
    {
        return decodeBrokerDiagnostic(rx.tryReceive());
    };
    return api;
}

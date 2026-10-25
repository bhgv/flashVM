// Create a value-only AS2 endpoint; tx/rx are separately rooted FlashVM channel wrappers.
// Parameters: tx sends toward Dis, rx receives from Dis. Returns an owned helper object.
function makeUiBridge(tx, rx)
{
    var api = new Object(); // Keep methods in the UI context, never in the Dis heap.
    var digits = "0123456789abcdef"; // Canonical lowercase header alphabet.


    // Encode one numeric int32/uint32 bit pattern. Returns exactly eight ASCII digits.
    function word(value)
    {
        var result = ""; // Build the fixed-width word from the least significant digit.
        value = value >>> 0; // Convert the bit pattern to an unsigned AS Number.
        for (var i = 0; i < 8; i++) // Emit all eight digits, including leading zeros.
        {
            result = digits.charAt(value & 15) + result; // Prepend the next hexadecimal digit.
            value = value >>> 4; // Shift without propagating a signed high bit.
        } // Finish the fixed-width word.
        return result; // Return the independent header string.
    } // Finish the word encoder.


    // Count canonical UTF-8 bytes of a UTF-16 AS string. text is borrowed; returns its byte extent.
    function byteLength(text)
    {
        var size = 0; // Count bytes rather than AS UTF-16 code units.
        for (var i = 0; i < text.length; i++) // Inspect every input code unit.
        {
            var c = text.charCodeAt(i); // Read the current UTF-16 unit.
            if (c < 128) // ASCII, including NUL, occupies one byte.
            {
                size += 1; // Preserve embedded zero as payload data.
            } // Finish the ASCII case.
            else if (c < 2048) // The next Unicode range occupies two bytes.
            {
                size += 2; // Count the two-byte encoding.
            } // Finish the two-byte case.
            else if (c >= 55296 && c <= 56319 && i + 1 < text.length
                && text.charCodeAt(i + 1) >= 56320 && text.charCodeAt(i + 1) <= 57343)
            {
                size += 4; // A valid surrogate pair encodes one supplementary character.
                i += 1; // Consume its low surrogate exactly once.
            } // Finish the supplementary character case.
            else if (c >= 55296 && c <= 57343) // An isolated surrogate has no Unicode scalar value.
            {
                return -1; // Reject input that Limbo would otherwise have to replace.
            } // Finish invalid-surrogate rejection.
            else // Remaining BMP characters occupy three bytes.
            {
                size += 3; // The Dis codec additionally rejects a lossy Unicode conversion.
            } // Finish the remaining UTF-16 case.
        } // Finish byte counting.
        return size; // Return the transport quota measurement.
    } // Finish the byte-length helper.


    // Validate common fields. m is borrowed; returns true only for a bounded wire record.
    function valid(m)
    {
        if (m == null || typeof m.kind != "number" || m.kind < 1 || m.kind > 7 || m.kind != Math.floor(m.kind))
        {
            return false; // Reject unknown/nonintegral message classes.
        } // Finish the kind check.
        var names = ["id", "code", "flags", "first", "second"]; // Header field order.
        for (var i = 0; i < names.length; i++) // Validate before bitwise conversion truncates.
        {
            var n = m[names[i]]; // Inspect the original numeric argument.
            var low = i < 3 ? 0 : -2147483648; // Unsigned metadata versus signed arguments.
            var high = i < 3 ? 4294967295 : 2147483647; // Exact 32-bit bounds.
            if (typeof n != "number" || n != Math.floor(n) || n < low || n > high)
            {
                return false; // Reject NaN, infinity, fractions and narrowing.
            } // Finish the field check.
        } // Finish numeric validation.
        if ((m.kind <= 2 && m.id == 0) || (m.kind > 2 && m.id != 0))
        {
            return false; // Requests/replies correlate; events carry no request ID.
        } // Finish correlation validation.
        return typeof m.text == "string" && byteLength(m.text) >= 0 && byteLength(m.text) <= 1024; // Enforce byte quota.
    } // Finish common validation.


    // Encode m without queueing. Returns a string, or undefined for invalid fields.
    api.encode = function(m)
    {
        if (!valid(m)) // Keep invalid input out of the transport queue.
        {
            return undefined; // Caller can display or log its validation error.
        } // Finish the validation branch.
        return "fvd1:" + word(m.kind) + word(m.id) + word(m.code) + word(m.flags)
            + word(m.first) + word(m.second) + m.text; // Append counted text without delimiters.
    }; // Finish the encoder method.


    // Decode one received string. Returns a value record or undefined; never invokes UI code.
    api.decode = function(frame)
    {
        if (typeof frame != "string" || frame.length < 53 || frame.substr(0, 5) != "fvd1:")
        {
            return undefined; // Reject another protocol or a truncated header.
        } // Finish framing validation.
        var fields = new Array(); // Collect the six already bounded hexadecimal words.
        for (var i = 0; i < 6; i++) // Parse fixed positions, never payload delimiters.
        {
            var part = frame.substr(5 + i * 8, 8); // Borrow exactly one header word.
            for (var j = 0; j < 8; j++) // Reject parseInt's permissive partial-input behavior.
            {
                if (digits.indexOf(part.charAt(j)) < 0) // Admit lowercase wire digits only.
                {
                    return undefined; // Reject the entire message on any invalid digit.
                } // Finish digit rejection.
            } // Finish word validation.
            fields[i] = parseInt(part, 16); // All eight digits are now known to be valid.
        } // Finish header parsing.
        var m = new Object(); // Create a record in the receiving AS heap.
        m.kind = fields[0]; // Recover the operation class.
        m.id = fields[1]; // Recover the unsigned correlation ID.
        m.code = fields[2]; // Recover operation/status/key/button.
        m.flags = fields[3]; // Recover class-specific flags.
        m.first = fields[4] | 0; // Restore the signed first argument.
        m.second = fields[5] | 0; // Restore the signed second argument.
        m.text = frame.substr(53); // Preserve the entire payload, including zero and newlines.
        return valid(m) ? m : undefined; // Recheck kind, correlation and byte extent.
    }; // Finish the decoder method.


    // Try to send m without blocking the UI thread. Returns false on invalid input/backpressure.
    api.trySend = function(m)
    {
        var frame = api.encode(m); // Construct one atomic channel message.
        if (typeof frame == "undefined") // Avoid sending an invalid record.
        {
            return false; // Caller retains its original request for diagnosis or retry.
        } // Finish invalid-record handling.
        return tx.trySend(frame); // False means not accepted; retain and retry the request.
    }; // Finish the nonblocking send method.


    // Poll at most one reply/event. Returns undefined when empty or malformed; no UI mutation.
    api.tryReceive = function()
    {
        return api.decode(rx.tryReceive()); // The owner decides how to apply the returned record.
    }; // Finish the nonblocking receive method.
    api.byteLength = byteLength; // Expose transport byte counts for bounded application fragments.
    return api; // The caller retains the helper and its channel wrappers.
} // Finish the endpoint factory.

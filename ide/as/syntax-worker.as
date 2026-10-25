// Build a worker-local lexical colourizer; no MovieClip, TextField or Dis pointer is accessed.
// No parameters. Returns scan(job), a bounded pure-data operation suitable for a loaded AS worker.
function makeIdeSyntaxWorker()
{
    var api = new Object(); // Keep all worker state local to its own FlashVM context.
    var asWords = " break case catch class const continue default delete do dynamic else extends false"
        + " finally for function get if implements import in instanceof interface intrinsic is namespace"
        + " native new null override package private protected public return set static super switch this"
        + " throw true try typeof undefined use var void while with "; // AS keyword vocabulary.
    var limboWords = " adt alt array big break byte case chan con continue cyclic do else exit fn for hd"
        + " if implement import include int len list load module nil of or pick real ref return self"
        + " spawn string tagof tl to type while "; // Limbo keyword vocabulary.


    // Recognize an ASCII decimal digit. c is one UTF-16 code unit; returns a Boolean.
    function digit(c)
    {
        return c >= 48 && c <= 57; // Avoid locale-dependent classification.
    } // Finish decimal classification.


    // Recognize identifier starts for lexical colouring, not compiler acceptance.
    // c is one UTF-16 unit. Returns true for ASCII letters, _, $, or non-ASCII text.
    function nameStart(c)
    {
        return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95 || c == 36 || c >= 128; // Compiler diagnostics remain authoritative.
    } // Finish identifier-start classification.


    // Scan one bounded source window with an explicit incoming lexical state.
    // job carries source, language, revision, base, state and escaped. Returns spans/end state or ok=false.
    api.scan = function(job)
    {
        var result = new Object(); // Return values only; the UI owns all display objects.
        result.ok = false; // No partial result is admitted after validation/budget failure.
        if (job == null || typeof job.source != "string" || job.source.length > 16384
            || typeof job.revision != "number" || job.revision < 0 || job.revision > 2147483647
            || job.revision != Math.floor(job.revision) || typeof job.base != "number"
            || job.base < 0 || job.base > 2147483647 || job.base != Math.floor(job.base)
            || typeof job.ticket != "number" || job.ticket < 0 || job.ticket > 2147483647
            || job.ticket != Math.floor(job.ticket) || (job.language != "as" && job.language != "limbo"))
        {
            result.error = "invalid syntax window"; // Caller reports admission failure explicitly.
            return result; // Preserve the current UI colours.
        } // Finish source/version/range validation.
        var mode = job.state; // Carry multiline lexical context from the preceding window.
        if (mode != "normal" && mode != "line" && mode != "block" && mode != "single" && mode != "double")
        {
            result.error = "invalid lexical state"; // A viewport must supply a known checkpoint.
            return result; // Never guess the state at an arbitrary file offset.
        } // Finish state validation.
        var pendingStar = job.star == true; // Preserve a block-end delimiter split across windows.
        var escaped = job.escaped == true; // Preserve a trailing string escape across windows.
        var source = job.source; // Borrow the immutable job string in this worker.
        var offsets = new Array(); // Map UTF-16 boundaries to Unicode character offsets.
        var points = 0; // Count Dis-compatible Unicode character positions.
        var unit = 0; // Walk the worker's AS UTF-16 representation.
        while (unit < source.length)
        {
            offsets[unit] = points; // Publish the boundary before this character.
            var c = source.charCodeAt(unit); // Read the next UTF-16 unit.
            if (c >= 55296 && c <= 56319) // Supplementary Unicode needs a valid low surrogate.
            {
                if (unit + 1 >= source.length || source.charCodeAt(unit + 1) < 56320
                    || source.charCodeAt(unit + 1) > 57343)
                {
                    result.error = "split or isolated surrogate"; // Require scalar-aligned windows.
                    return result; // Do not publish shifted source ranges.
                } // Finish pair validation.
                offsets[unit + 1] = points; // Internal surrogate boundary is never an output token edge.
                unit += 2; // Consume one complete supplementary character.
            } // Finish supplementary decoding.
            else if (c >= 56320 && c <= 57343) // A low surrogate cannot start a character.
            {
                result.error = "isolated low surrogate"; // Match the shared AS/Limbo Unicode profile.
                return result; // Preserve the current presentation.
            } // Finish invalid-surrogate handling.
            else
            {
                unit += 1; // Consume one BMP character, including NUL.
            } // Finish BMP decoding.
            points += 1; // Dis character offsets count this scalar once.
        } // Finish boundary mapping.
        offsets[source.length] = points; // Include the final half-open boundary.
        if (points > 2147483647 - job.base)
        {
            result.error = "source offset overflow"; // Never wrap a document range.
            return result; // Reject the whole job.
        } // Finish document-range admission.
        var spans = new Array(); // Collect only coloured runs; default text remains unclassified.
        var words = job.language == "limbo" ? limboWords : asWords; // Select the lexical vocabulary.
        var i = 0; // Current UTF-16 scanning offset.
        while (i < source.length)
        {
            var start = i; // Save the beginning of this lexical run.
            var style = ""; // Empty style means no colour overlay.
            if (mode == "normal") // Establish a new token or a continued lexical state.
            {
                var ch = source.charCodeAt(i); // Inspect the next character once.
                var next = source.charAt(i + 1); // Read lookahead without indexing beyond the string.
                if ((source.charAt(i) == "/" && next == "/")
                    || (job.language == "limbo" && source.charAt(i) == "#"))
                {
                    mode = "line"; // Preserve line-comment context if the window ends early.
                } // Finish line-comment recognition.
                else if (source.charAt(i) == "/" && next == "*")
                {
                    pendingStar = false; // Opening delimiters do not inherit an older trailing star.
                    mode = "block"; // Block comments can span any number of windows.
                    i += 2; // Skip the opening delimiter before looking for its end.
                } // Finish block-comment recognition.
                else if (ch == 34 || ch == 39)
                {
                    mode = ch == 34 ? "double" : "single"; // Preserve the exact quote class.
                    escaped = false; // A fresh literal has no preceding escape.
                    i += 1; // Do not interpret its opening quote as the closing quote.
                } // Finish quoted-literal recognition.
                else if (nameStart(ch))
                {
                    i += ch >= 55296 && ch <= 56319 ? 2 : 1; // Advance by one Unicode scalar.
                    while (i < source.length && (nameStart(source.charCodeAt(i)) || digit(source.charCodeAt(i))))
                    {
                        var n = source.charCodeAt(i); // Inspect a potential supplementary name character.
                        i += n >= 55296 && n <= 56319 ? 2 : 1; // Keep token boundaries scalar-aligned.
                    } // Finish the identifier run.
                    style = words.indexOf(" " + source.substring(start, i) + " ") >= 0 ? "keyword" : "name"; // Colour by vocabulary only.
                } // Finish identifier recognition.
                else if (digit(ch))
                {
                    i += 1; // Begin a bounded lexical numeric run.
                    while (i < source.length && (digit(source.charCodeAt(i))
                        || source.charAt(i) == "." || source.charAt(i) == "x" || source.charAt(i) == "X"
                        || (source.charCodeAt(i) >= 65 && source.charCodeAt(i) <= 70)
                        || (source.charCodeAt(i) >= 97 && source.charCodeAt(i) <= 102)))
                    {
                        i += 1; // Compiler diagnostics decide whether this numeric spelling is valid.
                    } // Finish numeric scanning.
                    style = "number"; // Emit a colour range, not a parsed numeric value.
                } // Finish numeric-run recognition.
                else
                {
                    i += 1; // Leave punctuation and whitespace in the default style.
                } // Finish ordinary-character handling.
            } // Finish new-token recognition.
            if (mode == "line")
            {
                while (i < source.length && source.charAt(i) != "\n")
                {
                    i += 1; // Retain all text before the line terminator.
                } // Finish this comment fragment.
                style = "comment"; // Include its opening delimiter in the coloured span.
                if (i < source.length)
                {
                    mode = "normal"; // The next iteration consumes the line terminator normally.
                } // Finish line-state recovery.
            } // Finish continued line comments.
            else if (mode == "block")
            {
                while (i < source.length)
                {
                    if (pendingStar && source.charAt(i) == "/") // Complete a delimiter split by the window edge.
                    {
                        i += 1; // Consume the carried closing slash.
                        pendingStar = false; // Clear the consumed boundary state.
                        mode = "normal"; // Resume ordinary scanning after the comment.
                        break; // End this comment fragment here.
                    } // Finish cross-window delimiter recognition.
                    pendingStar = false; // A different next unit cannot close the carried star.
                    if (source.charAt(i) == "*" && source.charAt(i + 1) == "/")
                    {
                        i += 2; // Include both bytes of the closing delimiter.
                        mode = "normal"; // The following source starts a new token.
                        break; // Stop this comment run at its actual boundary.
                    } // Finish block-end recognition.
                    pendingStar = source.charAt(i) == "*"; // Carry a final star if no slash fits this window.
                    i += 1; // Continue across line endings without losing block state.
                } // Finish this block-comment fragment.
                style = "comment"; // Colour the whole fragment consistently.
            } // Finish continued block comments.
            else if (mode == "single" || mode == "double")
            {
                var quote = mode == "double" ? 34 : 39; // Retain the active literal delimiter.
                while (i < source.length)
                {
                    var q = source.charCodeAt(i); // Read one literal unit.
                    i += 1; // Advance before handling its terminating effect.
                    if (escaped)
                    {
                        escaped = false; // The escape consumes exactly this unit.
                    } // Finish escaped-unit handling.
                    else if (q == 92)
                    {
                        escaped = true; // Carry a final backslash into the next contiguous window.
                    } // Finish escape recognition.
                    else if (q == quote || q == 10)
                    {
                        mode = "normal"; // Recover at a closing quote or malformed raw newline.
                        break; // End this literal colour span.
                    } // Finish literal termination.
                } // Finish this literal fragment.
                style = "string"; // Rune literals and strings share the presentation style.
            } // Finish continued quoted literals.
            if (style != "" && i > start)
            {
                if (spans.length >= 4096)
                {
                    result.error = "syntax span budget"; // Keep adversarial tiny-token input bounded.
                    return result; // Never publish an incomplete overlay as complete.
                } // Finish output quota validation.
                var span = new Object(); // Allocate one plain-data range in the worker heap.
                span.first = job.base + offsets[start]; // Use document Unicode character coordinates.
                span.last = job.base + offsets[i]; // Keep the right endpoint exclusive.
                span.unitFirst = start; // Expose local UTF-16 offsets for AS text presentation.
                span.unitLast = i; // Preserve the original source slice exactly.
                span.style = style; // Let the UI choose colours independently of lexical work.
                spans.push(span); // Append in source order; no display object is touched.
            } // Finish one coloured run.
        } // Finish this bounded source window.
        result.ok = true; // All admission and output quotas have passed.
        result.revision = job.revision; // The UI must compare this before applying results.
        result.ticket = job.ticket; // Match the exact Dis-assigned viewport/checkpoint job.
        result.base = job.base; // The caller must also match the requested source window.
        result.length = points; // State the Unicode range covered by this result.
        result.spans = spans; // Transfer values, never worker-owned display pointers.
        result.state = mode; // A following contiguous window starts from this state.
        result.star = mode == "block" && pendingStar; // A following window can complete */ correctly.
        result.escaped = escaped; // Retain the pending string escape when necessary.
        return result; // The UI owner alone may apply the returned overlay.
    }; // Finish the worker scan operation.
    // Extract lexical completion/call context from a window ending at the caret.
    // job matches scan; optional job.context is a prior line/checkpoint. Returns value metadata only.
    api.context = function(job)
    {
        var scanned = api.scan(job); // Reuse exactly the same comment/string/Unicode classification.
        if (!scanned.ok)
        {
            return scanned; // Context never bypasses lexical admission or quotas.
        } // Finish scan admission.
        var context = {name: "", dot: false, stack: new Array()}; // New worker-owned continuation state.
        if (job.context != null)
        {
            if (job.context.stack == null || job.context.stack.length > 64
                || typeof job.context.name != "string" || job.context.name.length > 256)
            {
                return {ok: false, error: "invalid call checkpoint"}; // Bound imported continuation data.
            } // Finish checkpoint extent validation.
            context.name = job.context.name; // Copy the preceding callable/qualifier spelling.
            context.dot = job.context.dot == true; // Retain a qualified-member boundary.
            for (var c = 0; c < job.context.stack.length; c++)
            {
                var old = job.context.stack[c]; // Borrow one prior delimiter frame.
                if ((old.kind != "(" && old.kind != "[" && old.kind != "{")
                    || typeof old.name != "string" || old.name.length > 256
                    || typeof old.argument != "number" || old.argument < 0 || old.argument > 1024
                    || old.argument != Math.floor(old.argument))
                {
                    return {ok: false, error: "invalid call frame"}; // Reject malformed or unbounded call state.
                } // Finish delimiter-frame admission.
                context.stack.push({kind: old.kind, name: old.name, argument: old.argument}); // No mutable alias to another worker.
            } // Finish checkpoint copying.
        } // Finish continuation initialization.
        var index = 0; // Next colour span in source order.
        var i = 0; // Current UTF-16 source position.
        var point = job.base; // Current document Unicode character boundary.
        var prefix = null; // Completion metadata, absent outside an eligible name/member position.
        while (i < job.source.length)
        {
            var span = index < scanned.spans.length ? scanned.spans[index] : null; // Look up the next classified run.
            if (span != null && span.unitFirst == i)
            {
                var token = job.source.substring(span.unitFirst, span.unitLast); // Preserve original spelling/case.
                if (span.style == "name" || span.style == "keyword")
                {
                    var receiver = context.dot ? context.name : ""; // Keep unresolved receiver names for Dis to resolve.
                    var qualified = receiver == "" ? token : receiver + "." + token; // Builtin namespaces already resolve directly.
                    prefix = {text: token, receiver: receiver, qualified: qualified,
                        first: span.first, last: span.last}; // Dis decides actual insertion, not this worker.
                    context.name = span.style == "name" ? qualified : ""; // Keywords such as if are not callable names.
                    context.dot = false; // This identifier consumes the member separator.
                } // Finish name context.
                else
                {
                    prefix = null; // Do not complete inside numbers, strings or comments.
                    if (span.style != "comment")
                    {
                        context.name = ""; // A literal is not a callable identifier.
                        context.dot = false; // It cannot continue an earlier qualified name.
                    } // Finish literal context reset.
                } // Finish classified-run handling.
                i = span.unitLast; // Skip without rescanning comment/string contents as syntax.
                point = span.last; // Use the scanner's exact Dis-compatible boundary.
                index += 1; // Visit each classified run once.
                continue; // Resume at the next source boundary.
            } // Finish colour-span context processing.
            var ch = job.source.charAt(i); // Remaining source is unclassified punctuation/whitespace.
            i += 1; // Advance one ASCII punctuation or whitespace unit.
            point += 1; // Non-ASCII scalar runs were already consumed through spans.
            prefix = null; // Only a following dot can reopen member completion.
            if ((ch == "." || (job.language == "limbo" && ch == "-"
                && job.source.charAt(i) == ">")) && context.name != "")
            {
                if (ch == "-")
                {
                    i += 1; // Consume the second ASCII unit of Limbo module access.
                    point += 1; // Keep canonical Unicode replacement coordinates exact.
                } // Finish Limbo arrow handling.
                context.dot = true; // The next name is a member of this receiver.
                prefix = {text: "", receiver: context.name, qualified: context.name + ".",
                    first: point, last: point}; // Empty member prefix still offers a dropdown.
            } // Finish member separator handling.
            else if (ch == "(" || ch == "[" || ch == "{")
            {
                if (context.stack.length >= 64)
                {
                    return {ok: false, error: "call depth budget"}; // Bound deeply nested unfinished input.
                } // Finish delimiter budget validation.
                context.stack.push({kind: ch, name: ch == "(" ? context.name : "", argument: 0}); // Preserve nesting for comma counting.
                context.name = ""; // A new argument/container begins after this delimiter.
                context.dot = false; // Clear the prior member separator.
            } // Finish opening delimiter handling.
            else if (ch == ")" || ch == "]" || ch == "}")
            {
                var wanted = ch == ")" ? "(" : (ch == "]" ? "[" : "{"); // Match the exact container class.
                if (context.stack.length == 0 || context.stack[context.stack.length - 1].kind != wanted)
                {
                    return {ok: false, error: "unmatched call delimiter"}; // Compiler recovery remains authoritative.
                } // Finish closing delimiter admission.
                context.stack.pop(); // Restore the outer active argument context.
                context.name = ""; // A dynamic call result is not assigned an invented receiver type.
                context.dot = false; // Type inference belongs to Dis/compiler providers.
            } // Finish closing delimiter handling.
            else if (ch == ",")
            {
                if (context.stack.length > 0)
                {
                    var top = context.stack[context.stack.length - 1]; // Only the immediate call frame owns this comma.
                    if (top.kind == "(" && top.name != "")
                    {
                        top.argument += 1; // Array/object/nested-call commas do not advance the outer parameter.
                        if (top.argument > 1024)
                        {
                            return {ok: false, error: "argument count budget"}; // Match bounded call-tip admission.
                        } // Finish argument quota enforcement.
                    } // Finish call argument advancement.
                } // Finish delimiter-context lookup.
                context.name = ""; // The next argument starts without an inferred callable name.
                context.dot = false; // Clear any earlier member chain.
            } // Finish comma handling.
            else if (ch.charCodeAt(0) > 32)
            {
                context.name = ""; // Operators terminate the current qualified name.
                context.dot = false; // Do not complete across unrelated expressions.
            } // Finish ordinary operator handling.
        } // Finish caret-prefix traversal.
        if (scanned.state != "normal")
        {
            prefix = null; // Suppress name suggestions while typing inside a comment/literal.
        } // Finish lexical-mode completion gating.
        var call = null; // No parameter tip outside an admitted open call.
        for (var depth = context.stack.length - 1; depth >= 0; depth--)
        {
            var frame = context.stack[depth]; // Look outward through grouping/container delimiters.
            if (frame.kind == "(" && frame.name != "")
            {
                call = {name: frame.name, argument: frame.argument}; // Select the innermost actual call.
                break; // Outer function tips resume after this call closes.
            } // Finish active-call selection.
        } // Finish call-context selection.
        return {ok: true, revision: job.revision, ticket: job.ticket, base: job.base,
            prefix: prefix, call: call, context: context}; // Dis resolves symbols and chooses edits/help from this value.
    }; // Finish lexical assistance context extraction.
    return api; // Each worker gets its own helper and lexical vocabulary.
} // Finish the worker factory.

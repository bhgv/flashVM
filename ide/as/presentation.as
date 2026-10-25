// Create a UI-owner presentation adapter; target methods draw values but own no editor policy.
// codec is makeUiBridge(...); target supplies document, selection, syntax, completion and fault callbacks.
// Returns frame acceptance and stale-result gates; documents/history/commands remain Dis-owned.
function makeIdePresentation(codec, target)
{
    var api = new Object(); // Expose only presentation operations to the UI owner.
    var revision = -1; // Latest fully displayed Dis snapshot revision.
    var characters = 0; // Unicode length of the fully displayed document.
    var assembly = null; // At most one bounded in-flight snapshot on this ordered endpoint.
    var syntaxTicket = null; // Exact Dis-assigned viewport request currently eligible for display.
    var completionTicket = null; // Exact completion context currently eligible for display.
    var calltipTicket = null; // Exact Dis-selected overload/active-argument context.
    var active = false; // Reject callbacks that reenter snapshot publication.
    var digits = "0123456789abcdef"; // Application metadata uses the shared canonical alphabet.


    // Report a concrete protocol error. message is borrowed; returns false without UI mutation.
    function reject(message)
    {
        target.fault(message); // The application decides how to surface the diagnostic.
        return false; // Never treat malformed data as a successful frame.
    } // Finish diagnostic delivery.


    // Decode one eight-character metadata word. text/start name a bounded ASCII field.
    // Returns uint32 or undefined; parseInt is called only after all eight digits validate.
    function word(text, start)
    {
        var part = text.substr(start, 8); // Keep payload text out of numeric parsing.
        if (part.length != 8)
        {
            return undefined; // Reject a truncated metadata field.
        } // Finish extent validation.
        for (var i = 0; i < 8; i++)
        {
            if (digits.indexOf(part.charAt(i)) < 0)
            {
                return undefined; // Reject permissive partial parseInt input.
            } // Finish one digit check.
        } // Finish word validation.
        return parseInt(part, 16); // Return the complete validated bit pattern.
    } // Finish application metadata decoding.


    // Validate/assemble one decoded application frame before invoking display callbacks.
    // frame is a value record; returns true for an accepted fragment/presentation, false otherwise.
    function acceptFrame(frame)
    {
        if (frame == null || (frame.kind != 2 && frame.kind != 7))
        {
            return reject("unexpected IDE presentation class"); // Input/commands cannot mutate the view directly.
        } // Finish direction/profile validation.
        if (frame.flags < revision)
        {
            return false; // A late snapshot/selection must not roll the display backwards.
        } // Finish revision freshness check.
        if (frame.kind == 7 && frame.code == 65536)
        {
            if (assembly != null || frame.flags != revision || frame.first < 0 || frame.second < 0
                || frame.first > characters || frame.second > characters)
            {
                return reject("selection without its complete snapshot"); // Keep document and cursor atomic in order.
            } // Finish selection admission.
            target.selection(frame.first, frame.second, revision); // Apply Dis-decided Unicode offsets only.
            return true; // No edit/history operation is performed in AS.
        } // Finish selection presentation.
        if (frame.text.length < 56 || frame.first < 0 || frame.first > 1
            || frame.second < 0 || frame.second > frame.flags || frame.flags > 2147483647)
        {
            return reject("invalid snapshot metadata"); // Reject malformed owner state before assembling text.
        } // Finish snapshot scalar validation.
        var values = new Array(); // Read all seven words, including signed navigation results.
        for (var i = 0; i < 7; i++)
        {
            values[i] = word(frame.text, i * 8); // Fixed positions are independent of payload characters.
            if (typeof values[i] == "undefined")
            {
                return reject("invalid snapshot hexadecimal field"); // Do not accept a partial header.
            } // Finish metadata word validation.
        } // Finish metadata parsing.
        var offset = values[5]; // Byte offset in the UTF-8 document snapshot.
        var total = values[6]; // Exact total UTF-8 bytes for this snapshot.
        var text = frame.text.substr(56); // Preserve all Unicode/NUL content after the ASCII metadata.
        var size = codec.byteLength(text); // Measure transport bytes, not UTF-16 units.
        if (size < 0 || total > 4194304 || offset > total || size > total - offset)
        {
            return reject("snapshot fragment byte extent"); // Bound memory before retaining a fragment.
        } // Finish payload extent validation.
        if (assembly == null)
        {
            if (offset != 0)
            {
                return reject("snapshot starts with a missing fragment"); // Never guess missing source bytes.
            } // Finish first-fragment validation.
            assembly = new Object(); // Retain immutable pieces until the whole snapshot is ready.
            assembly.header = frame; // Preserve exact revision/status/owner state across fragments.
            assembly.total = total; // Fix the admitted full extent.
            assembly.offset = 0; // Track the next expected byte offset.
            assembly.parts = new Array(); // Join once to avoid quadratic string concatenation.
            assembly.metadata = frame.text.substr(0, 40); // Require stable navigation metadata too.
        } // Finish new snapshot construction.
        if (offset != assembly.offset || total != assembly.total
            || frame.kind != assembly.header.kind || frame.id != assembly.header.id
            || frame.flags != assembly.header.flags || frame.code != assembly.header.code
            || frame.first != assembly.header.first || frame.second != assembly.header.second
            || frame.text.substr(0, 40) != assembly.metadata || (size == 0 && offset != total))
        {
            assembly = null; // Discard the damaged assembly explicitly after reporting failure.
            return reject("snapshot fragments changed identity or order"); // Earlier displayed state remains valid.
        } // Finish fragment consistency validation.
        assembly.parts.push(text); // Retain an independent string value, never a native buffer pointer.
        assembly.offset += size; // Advance only after all checks pass.
        if (assembly.offset != total)
        {
            return true; // Wait for another bounded frame; do not expose partial text.
        } // Finish incomplete-snapshot handling.
        var complete = assembly.parts.join(""); // Materialize the full snapshot exactly once.
        assembly = null; // Release temporary fragment references before presentation callbacks.
        characters = 0; // Recount only when publishing a completed authoritative snapshot.
        for (var unit = 0; unit < complete.length; unit++)
        {
            var c = complete.charCodeAt(unit); // Canonical wire text already guarantees valid pairs.
            if (c >= 55296 && c <= 56319)
            {
                unit += 1; // Count a supplementary character once, as Dis does.
            } // Finish supplementary handling.
            characters += 1; // Record one Unicode character boundary.
        } // Finish source coordinate counting.
        if (characters > 1048576)
        {
            return reject("document character budget"); // Match the actual document owner's bound.
        } // Finish completed-source admission.
        if (revision != frame.flags)
        {
            syntaxTicket = null; // Changed text invalidates older lexical jobs.
            completionTicket = null; // Changed text invalidates earlier completion context.
            calltipTicket = null; // Changed source invalidates earlier parameter help.
        } // Finish source-revision transition.
        revision = frame.flags; // Adopt the owner revision represented by the completed text.
        target.document(complete, revision, frame.first, frame.second, frame.code); // Render an authoritative Dis snapshot.
        return true; // Successful presentation is independent from any edit command execution.
    } // Finish one application-frame operation.


    // Apply one decoded frame on the UI owner. frame is borrowed; returns its admission result.
    api.accept = function(frame)
    {
        if (active)
        {
            return false; // Do not recursively publish a snapshot from inside its callbacks.
        } // Finish reentry rejection.
        active = true; // Protect the complete publication sequence.
        try
        {
            return acceptFrame(frame); // Preserve the exact success/error result.
        }
        finally
        {
            active = false; // Release the guard even when a display callback throws.
        } // Finish publication cleanup.
    }; // Finish the owner-frame entry point.


    // Register one Dis-selected syntax job. ticket carries revision/base/ticket identity.
    api.expectSyntax = function(ticket)
    {
        syntaxTicket = {revision: ticket.revision, base: ticket.base, ticket: ticket.ticket}; // Copy mutable caller metadata.
    }; // Finish syntax eligibility publication.


    // Display only a matching successful worker result. result is borrowed; returns false when stale.
    api.applySyntax = function(result)
    {
        if (result == null || !result.ok || syntaxTicket == null || result.revision != revision
            || result.revision != syntaxTicket.revision || result.base != syntaxTicket.base
            || result.ticket != syntaxTicket.ticket)
        {
            return false; // A late independent worker cannot recolour newer/different source.
        } // Finish syntax context validation.
        if (result.spans == null || result.spans.length > 4096 || result.length < 0
            || result.length > 16384 || result.base < 0 || result.base + result.length > characters)
        {
            return false; // Never hand an unbounded or out-of-document overlay to a renderer.
        } // Finish lexical extent validation.
        var last = result.base; // Enforce nonoverlapping ordered Unicode ranges.
        for (var i = 0; i < result.spans.length; i++)
        {
            var span = result.spans[i]; // Inspect one worker-owned value record.
            if (span.first < last || span.last <= span.first || span.last > result.base + result.length
                || span.unitFirst < 0 || span.unitLast <= span.unitFirst || span.unitLast > 16384)
            {
                return false; // Preserve the previous overlay on malformed geometry.
            } // Finish one range admission.
            last = span.last; // Advance the disjoint-range boundary.
        } // Finish lexical range validation.
        return target.syntax(result) === true; // Unsupported range renderers must explicitly refuse.
    }; // Finish syntax presentation.


    // Register a completion context assigned by Dis. ticket is copied rather than retained mutable.
    api.expectCompletion = function(ticket)
    {
        completionTicket = {revision: ticket.revision, generation: ticket.generation,
            prefix: ticket.prefix, ticket: ticket.ticket}; // Bind index generation and editor context together.
    }; // Finish completion eligibility publication.


    // Display a bounded candidate result matching the current Dis context, without computing edits.
    api.applyCompletion = function(result)
    {
        if (result == null || completionTicket == null || result.revision != revision
            || result.revision != completionTicket.revision || result.generation != completionTicket.generation
            || result.prefix != completionTicket.prefix || result.ticket != completionTicket.ticket
            || result.items == null || result.items.length > 64)
        {
            return false; // Ignore stale index/source/caret context rather than offering the wrong insertion.
        } // Finish completion context validation.
        return target.completion(result.items) === true; // The popup backend acknowledges actual presentation.
    }; // Finish candidate presentation.
    // Register a Dis-selected call-help context, including active argument and catalog generation.
    api.expectCalltip = function(ticket)
    {
        calltipTicket = {revision: ticket.revision, generation: ticket.generation,
            name: ticket.name, argument: ticket.argument, ticket: ticket.ticket}; // Copy context values.
    }; // Finish help-context publication.


    // Present matching overload/parameter documentation. result is copied value data, not executable UI.
    api.applyCalltip = function(result)
    {
        if (result == null || calltipTicket == null || result.revision != revision
            || result.revision != calltipTicket.revision || result.generation != calltipTicket.generation
            || result.name != calltipTicket.name || result.argument != calltipTicket.argument
            || result.ticket != calltipTicket.ticket || result.tips == null || result.tips.length > 64)
        {
            return false; // A late reply cannot highlight a parameter in a different active call.
        } // Finish call-help context validation.
        return target.calltip(result.tips) === true; // Backend displays signature and the selected argument's short help.
    }; // Finish help presentation.
    return api; // The caller keeps this adapter only on the UI owner thread.
} // Finish the presentation factory.

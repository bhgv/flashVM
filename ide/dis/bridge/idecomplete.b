implement Idecomplete;
include "idecomplete.m";


# Clone one admitted provider value, preventing mutable ADT aliasing across the index boundary.
# item is borrowed/non-nil; returns a fresh Item reference with immutable string values.
copy(item: ref Item): ref Item
{
    parameters := array[len item.parameters] of ref Parameter;
    for (i := 0; i < len parameters; i++) {
        p := item.parameters[i];
        parameters[i] = ref Parameter(p.name, p.type_name, p.optional, p.summary);
    }
    return ref Item(item.name, item.kind, item.signature, item.documentation, parameters);
}


# Define deterministic ordering independent of provider arrival order.
# a/b are admitted items. Returns one when a sorts before or equal to b.
before(a, b: ref Item): int
{
    if (a.name != b.name) {
        return a.name < b.name;
    }
    if (a.kind != b.kind) {
        return a.kind < b.kind;
    }
    return a.signature <= b.signature;
}


# Stable bottom-up merge sort avoids quadratic publication work on a large symbol batch.
# items is exclusively owned; returns a sorted array containing the same private Item references.
sort(items: array of ref Item): array of ref Item
{
    workspace := array[len items] of ref Item;
    for (width := 1; width < len items; width *= 2) {
        for (base := 0; base < len items; base += 2 * width) {
            middle := base + width;
            if (middle > len items) {
                middle = len items;
            }
            last := base + 2 * width;
            if (last > len items) {
                last = len items;
            }
            left := base;
            right := middle;
            for (target := base; target < last; target++) {
                if (right == last || (left < middle && before(items[left], items[right]))) {
                    workspace[target] = items[left];
                    left++;
                } else {
                    workspace[target] = items[right];
                    right++;
                }
            }
        }
        (items, workspace) = (workspace, items);
    }
    return items;
}


# Bound UTF-8 conversion before allocating a temporary encoded field.
# value is borrowed text. Returns its byte count, or -1 when the field exceeds the quota.
fieldbytes(value: string): int
{
    if (len value > MaxFieldBytes) {
        return -1;
    }
    bytes := len array of byte value;
    if (bytes > MaxFieldBytes) {
        return -1;
    }
    return bytes;
}


# Validate/copy an entire symbol batch before it replaces the owner's current generation.
# source is provider-owned and borrowed. Returns (status, private sorted array).
admit(source: array of ref Item): (int, array of ref Item)
{
    if (len source > MaxEntries) {
        return (TooLarge, nil);
    }
    items := array[len source] of ref Item;
    total := 0;
    for (i := 0; i < len source; i++) {
        item := source[i];
        if (item == nil || item.name == "" || item.kind < 0) {
            return (Invalid, nil);
        }
        namebytes := fieldbytes(item.name);
        signaturebytes := fieldbytes(item.signature);
        docbytes := fieldbytes(item.documentation);
        if (namebytes < 0 || signaturebytes < 0 || docbytes < 0 || len item.parameters > MaxParameters) {
            return (TooLarge, nil);
        }
        total += namebytes + signaturebytes + docbytes;
        for (j := 0; j < len item.parameters; j++) {
            p := item.parameters[j];
            if (p == nil || p.name == "" || (p.optional != 0 && p.optional != 1)) {
                return (Invalid, nil);
            }
            a := fieldbytes(p.name);
            b := fieldbytes(p.type_name);
            c := fieldbytes(p.summary);
            if (a < 0 || b < 0 || c < 0) {
                return (TooLarge, nil);
            }
            total += a + b + c;
        }
        if (total > MaxCatalogBytes) {
            return (TooLarge, nil);
        }
        items[i] = copy(item);
    }
    items = sort(items);
    for (i = 1; i < len items; i++) {
        a := items[i - 1];
        b := items[i];
        if (a.name == b.name && a.kind == b.kind && a.signature == b.signature) {
            return (Invalid, nil);
        }
    }
    return (Ok, items);
}


# Find the first possible name by binary search, then copy only bounded matching records.
# items is the private sorted index, prefix/limit are admitted query values. Returns a fresh result array.
query(items: array of ref Item, prefix: string, limit: int): array of ref Item
{
    low := 0;
    high := len items;
    while (low < high) {
        middle := low + (high - low) / 2;
        if (items[middle].name < prefix) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    last := low;
    while (last < len items && last - low < limit) {
        name := items[last].name;
        if (len name < len prefix || name[0:len prefix] != prefix) {
            break;
        }
        last++;
    }
    result := array[last - low] of ref Item;
    for (i := low; i < last; i++) {
        result[i - low] = copy(items[i]);
    }
    return result;
}


# Own all index state and serialize queries/publications through typed Dis channels.
# commands is owner-held until an acknowledged Shutdown. Returns nothing after shutdown.
serve(commands: chan of Request)
{
    generation := 0;
    items: array of ref Item;
    for (;;) {
        request := <-commands;
        if (request.reply == nil) {
            continue;
        }
        status := Ok;
        result: array of ref Item;
        terminal := 0;
        if (request.id < 0) {
            status = Invalid;
        } else if (request.operation == Publish) {
            if (request.generation <= generation) {
                status = Conflict;
            } else {
                (admitted, values) := admit(request.items);
                status = admitted;
                if (status == Ok) {
                    items = values;
                    generation = request.generation;
                }
            }
        } else if (request.operation == Query) {
            if (request.generation != generation) {
                status = Conflict;
            } else if (request.limit < 1 || request.limit > MaxResults) {
                status = Invalid;
            } else if (fieldbytes(request.prefix) < 0) {
                status = TooLarge;
            } else {
                result = query(items, request.prefix, request.limit);
            }
        } else if (request.operation == Shutdown) {
            terminal = 1;
        } else {
            status = Invalid;
        }
        request.reply <-= Reply(request.id, status, generation, result);
        if (terminal) {
            return;
        }
    }
}


# Create a distinct index owner using standard Dis spawn/rendezvous semantics.
# No parameters. Returns the new request channel; no OS or UI module is imported.
start(): chan of Request
{
    commands := chan of Request;
    spawn serve(commands);
    return commands;
}

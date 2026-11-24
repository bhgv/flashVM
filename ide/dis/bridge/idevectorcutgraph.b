# Copyright (c) 2026 bhgv
# EN: Compose retained Bézier intervals into editable boundaries.
# RU: Собирать сохранённые участки Безье в редактируемые контуры.


# EN: Compute the signed winding contribution of one borrowed curve at p.
# Split only when Y is non-monotonic, then bisect its unique ray crossing.
# RU: Вычислить знаковый вклад сегмента в обход точки p. При немонотонности
# разделить по Y, затем делением пополам найти единственное пересечение луча.
cutray(edge: CutCurve, p: CutPoint, depth: int): int
{
    box := cutbounds(edge);
    if (p.y < box.top || p.y >= box.bottom || p.x >= box.right) {
        return 0;
    }
    up := edge.a.y <= edge.b.y && edge.b.y <= edge.c.y && edge.c.y <= edge.d.y;
    down := edge.a.y >= edge.b.y && edge.b.y >= edge.c.y && edge.c.y >= edge.d.y;
    if (!up && !down && depth < 12) {
        (left, right) := cuthalf(edge);
        return cutray(left, p, depth+1)+cutray(right, p, depth+1);
    }
    direction := 0;
    if (edge.a.y <= p.y && p.y < edge.d.y) {
        direction = 1;
    } else if (edge.d.y <= p.y && p.y < edge.a.y) {
        direction = -1;
    }
    if (direction == 0 || p.x < box.left) {
        return direction;
    }
    if (edge.verb == Segment) {
        t := (p.y-edge.a.y)/(edge.d.y-edge.a.y);
        return direction*(cutlerp(edge.a, edge.d, t).x > p.x);
    }
    lo := 0.0;
    hi := 1.0;
    for (i := 0; i < 20; i++) {
        t := (lo+hi)*0.5;
        (q, ignored) := cuteval(edge, t);
        if ((q.y <= p.y) == (direction > 0)) {
            lo = t;
        } else {
            hi = t;
        }
    }
    (q, ignored) := cuteval(edge, (lo+hi)*0.5);
    return direction*(q.x > p.x);
}


# EN: Validated input outlines and their IDs in this operation's shared space.
# RU: Проверенные контуры и их номера в общем пространстве операции.
CutSources: adt
{
    outlines: array of ref CutOutline; # One normalized fill boundary per source.
    offsets: array of int;              # First global edge ID for each source.
    count: int;                         # Total number of source edges.
};

CutSubtract, CutUnion, CutIntersect: con iota; # Fill combination in the shared graph.

CutSpanLimit: con 1200;       # Maximum prepared vector edges in one composition.
CutCommandLimit: con 1328;    # SVG commands admitted including Move and Close.


# EN: Check whether the active span runs on another source's boundary.
# Return the side of that source's fill relative to the active orientation.
# RU: Проверить совпадение участка с границей другого источника. Вернуть
# сторону чужой заливки относительно направления текущего участка.
cutseamside(contacts: ref CutContacts, edge, first, end: int, t: real): int
{
    index := contacts.index;
    for (slot := index.heads[edge]; slot >= 0; slot = index.next[slot]) {
        seam := contacts.seams[slot/2];
        other := -1;
        if (seam.first == edge && seam.firstlo < t && t < seam.firsthi) {
            other = seam.second;
        } else if (seam.second == edge && seam.otherlo < t && t < seam.otherhi) {
            other = seam.first;
        }
        if (other >= first && other < end) {
            return 2*seam.same-1;
        }
    }
    return 0;
}


# EN: Classify both sides of the active interval. Common edges use their
# recorded direction. Open intervals between contacts use ordinary winding.
# Return +1 to keep, -1 to reverse, zero to drop, or 2 for ambiguity.
# RU: Классифицировать обе стороны участка. Общая кромка использует
# сохранённое направление, остальные участки используют правило обхода.
# +1 сохраняет направление, -1 обращает, ноль удаляет, 2 означает сомнение.
cutclassify(sources: CutSources, contacts: ref CutContacts, owner, index: int, lo, hi: real, operation: int): int
{
    source := sources.outlines[owner];
    t := (hi+lo)*0.5;
    (point, tangent) := cuteval(source.edges[index], t);
    edge := sources.offsets[owner]+index;
    right := 0;
    left := 0;
    for (i := 0; i < len sources.outlines; i++) {
        r := 1;
        l := 0;
        other := sources.outlines[i];
        start := sources.offsets[i];
        side := 0;
        if (i != owner) {
            side = cutseamside(contacts, edge, start, start+len other.edges, t);
        }
        if (other.internal && (i == owner || side != 0)) {
            valid: int;
            (r, l, valid) = cutrawsides(other, start, edge, point, tangent, t, contacts);
            if (!valid) {
                return 2;
            }
        } else if (i != owner) {
            if (side == 0) {
                r = cutsourcewinding(other, point) != 0;
                l = r;
            } else {
                r = side > 0;
                l = side < 0;
            }
        }
        if (operation == CutUnion) {
            right |= r;
            left |= l;
            if (right && left) {
                return 0;
            }
        } else if (i == 0) {
            (right, left) = (r, l);
        } else if (operation == CutSubtract) {
            right &= !r;
            left &= !l;
        } else {
            right &= r;
            left &= l;
        }
    }
    return right-left;
}


# EN: Find the next interior event on a source edge after lo. Original
# endpoints already share canonical IDs, regardless of endpoint events.
# RU: Найти следующее внутреннее событие ребра после lo. Исходные концы
# уже связаны общими номерами независимо от событий на концах.
cutnext(hits: array of CutHit, index: ref CutChain, edge: int, lo: real): (int, real)
{
    at := -1;
    hi := 1.0;
    for (slot := index.heads[edge]; slot >= 0; slot = index.next[slot]) {
        event := slot/2;
        hit := hits[event];
        t := hit.u;
        if (slot % 2 == 0) {
            t = hit.t;
        }
        if (t > lo+1.0e-9 && t < hi && t < 1.0-1.0e-9) {
            (at, hi) = (event, t);
        }
    }
    return (at, hi);
}


# EN: Feed retained fragments of one outline into the shared graph with
# original anchor IDs, separately from geometric endpoint aliases.
# RU: Передать фрагменты одного контура в общий граф с исходными номерами
# узлов, отдельно от общих геометрических концов.
cutkeep(boundary: ref CutBoundary, sources: CutSources, contacts: ref CutContacts, index: ref CutChain,
    owner, operation: int): int
{
    source := sources.outlines[owner];
    shift := sources.offsets[owner];
    hits := contacts.search.hits[0:contacts.search.count];
    for (i := 0; i < len source.edges; i++) {
        checkwork(contacts.search.work);
        lo := 0.0;
        from := shift+i;
        initial := from;
        while (lo < 1.0) {
            (hit, hi) := cutnext(hits, index, shift+i, lo);
            terminal := shift+source.next[i];
            destination := terminal;
            if (hit >= 0) {
                destination = sources.count+hit;
                terminal = -1;
            }
            side := cutclassify(sources, contacts, owner, i, lo, hi, operation);
            if (side == 2) {
                return 0;
            }
            if (side != 0 && !cutinsert(boundary, source.edges[i], from, destination, initial, terminal, lo, hi, side < 0)) {
                return 0;
            }
            lo = hi;
            from = destination;
            initial = -1;
        }
    }
    return 1;
}


# EN: Build and serialize the boundary of prepared real-coordinate outlines.
# The brush also uses this entry before any intermediate integer conversion.
# RU: Собрать и записать границу подготовленных вещественных контуров.
# Кисть использует эту точку входа до промежуточного округления.
cutcompose(sources: CutSources, contacts: ref CutContacts, operation: int): ref CutResult
{
    if (!cutbroadphase(sources, contacts)) {
        return nil;
    }
    search := contacts.search;
    index := cutincidence(contacts, sources.count);
    boundary := cutboundary(sources, search);
    for (i := 0; i < len sources.outlines; i++) {
        cutringindex(sources.outlines[i], search.work);
    }
    for (i = 0; i < len sources.outlines; i++) {
        if (!cutkeep(boundary, sources, contacts, index, i, operation)) {
            return nil;
        }
    }
    return cutloops(boundary, contacts.math, search.work);
}


# EN: Allocate a single bounded solver for the prepared workload. Incoming
# command counts are only an upper-bound hint for the event tables.
# RU: Выделить один ограниченный решатель. Число исходных команд задаёт
# верхнюю подсказку для таблиц пересечений и совпадений.
cutcontactbudget(hint: int, work: ref Work, math: Math): ref CutContacts
{
    if (hint > CutSpanLimit) {
        hint = CutSpanLimit;
    }
    search := ref CutSearch(array[96+6*hint] of CutHit, 0, 40000+300*hint, work);
    return ref CutContacts(search, array[48+hint] of CutSeam, 0, math, nil);
}


# EN: Allocate graph storage and copy positions before aliasing. The upper
# bound is one source span plus at most two splits per crossing/contact.
# RU: Выделить граф и скопировать позиции до объединения номеров.
# Каждый сегмент даёт один участок, событие добавляет не больше двух.
cutboundary(sources: CutSources, search: ref CutSearch): ref CutBoundary
{
    size := sources.count+search.count;
    spans := sources.count+2*search.count;
    boundary := ref CutBoundary(array[size] of CutPoint, array[size] of int,
        array[sources.count] of int, array[spans] of CutArc, array[size] of {* => -1},
        array[spans] of int, 0);
    for (i := 0; i < len sources.outlines; i++) {
        source := sources.outlines[i];
        shift := sources.offsets[i];
        for (j := 0; j < len source.edges; j++) {
            boundary.points[shift+j] = source.edges[j].a;
        }
        boundary.modes[shift:] = source.modes;
    }
    for (i = 0; i < search.count; i++) {
        boundary.points[sources.count+i] = search.hits[i].at;
    }
    cutaliases(boundary, search.work);
    return boundary;
}


# EN: Unite borrowed filled paths or subtract the second from the first.
# Resolve eligible contacts before publishing private editable contours.
# Nil defers ambiguous topology or an exhausted budget to coverage.
# RU: Объединить заливки или вычесть вторую из первой. Обработать касания
# до публикации редактируемых контуров. Nil оставляет неоднозначную
# топологию или исчерпанный бюджет обработке по покрытию.
cutboolean(paths: array of CutResult, unite: int, work: ref Work): ref CutResult
{
    if (len paths < 2 || len paths > 16 || (!unite && len paths != 2)) {
        return nil;
    }
    math := load Math Math->PATH;
    if (math == nil) {
        return nil;
    }
    hint := 0;
    for (i := 0; i < len paths; i++) {
        hint += len paths[i].nodes;
    }
    contacts := cutcontactbudget(hint, work, math);
    search := contacts.search;
    sources := CutSources(array[len paths] of ref CutOutline, array[len paths] of int, 0);
    for (i = 0; i < len paths; i++) {
        outline := cutoutline(paths[i], search, math);
        if (outline == nil || sources.count+len outline.edges > CutSpanLimit) {
            return nil;
        }
        sources.outlines[i] = outline;
        sources.offsets[i] = sources.count;
        sources.count += len outline.edges;
    }
    operation := CutSubtract;
    if (unite) {
        operation = CutUnion;
    }
    return cutcompose(sources, contacts, operation);
}

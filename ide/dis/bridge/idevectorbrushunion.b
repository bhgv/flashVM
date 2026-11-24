# Copyright (c) 2026 bhgv
# EN: Sweep one fixed four-cubic round nib through a general polylined stroke.
# RU: Провести один круглый отпечаток из четырёх кубик по сложному мазку.


# EN: A nonempty centerline interval in lexicographic endpoint order.
# RU: Непустой интервал осевой линии с упорядоченными концами.
BrushInterval: adt
{
    first: Point; # Lexicographically first endpoint.
    last: Point;  # Lexicographically last endpoint.
};


# EN: Temporary real-coordinate rings of flat strips and circular stamps.
# RU: Временные вещественные контуры полос и круглых отпечатков.
BrushVector: adt
{
    edges: array of CutCurve; # Directed edges, filled on the right.
    next: array of int;       # End vertex within each ring.
    count: int;               # Initialized entries.
};


# EN: Compare two copied page positions along their lexicographic order.
# RU: Сравнить две позиции листа лексикографически.
brushless(a, b: Point): int
{
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}


# EN: True when two positions have identical logical coordinates.
# RU: Проверить совпадение логических координат позиций.
brushequal(a, b: Point): int
{
    return a.x == b.x && a.y == b.y;
}


# EN: Union overlapping collinear intervals of the same nib sweep. Returning
# zero leaves perpendicular or separated intervals independent.
# RU: Объединить перекрывающиеся интервалы одного отрезка осевой линии.
# Ноль оставляет раздельные или неколлинеарные интервалы без изменений.
brushcombine(a, b: BrushInterval): (int, BrushInterval)
{
    dx := a.last.x-a.first.x;
    dy := a.last.y-a.first.y;
    if (brushless(a.last, b.first) || brushless(b.last, a.first) ||
        dx*(b.first.y-a.first.y) != dy*(b.first.x-a.first.x) ||
        dx*(b.last.y-a.first.y) != dy*(b.last.x-a.first.x)) {
        return (0, a);
    }
    if (brushless(b.first, a.first)) {
        a.first = b.first;
    }
    if (brushless(a.last, b.last)) {
        a.last = b.last;
    }
    return (1, a);
}


# EN: Consolidate exact retracing and collinear neighbours before arranging
# the filled sweep. Moving back through painted ink contributes one interval.
# RU: Объединить точный обратный ход и коллинеарные соседние участки.
# Повторный проход по осевой линии оставляет один интервал.
brushintervals(path: array of Point, work: ref Work): array of BrushInterval
{
    intervals := array[len path-1] of BrushInterval;
    count := 0;
    for (i := 1; i < len path; i++) {
        checkwork(work);
        a := path[i-1];
        b := path[i];
        if (brushequal(a, b)) {
            continue;
        }
        if (brushless(b, a)) {
            (a, b) = (b, a);
        }
        candidate := BrushInterval(a, b);
        j := 0;
        while (j < count) {
            (same, combined) := brushcombine(candidate, intervals[j]);
            if (!same) {
                j++;
                continue;
            }
            candidate = combined;
            for (k := j+1; k < count; k++) {
                intervals[k-1] = intervals[k];
            }
            count--;
            j = 0;
        }
        intervals[count++] = candidate;
    }
    return intervals[0:count];
}


# EN: Make the one nib used at every knot. The standard cubic quarter-arc
# handles remain in real coordinates until the final SVG is serialized.
# RU: Построить один отпечаток для всех узлов. Ручки кубических четвертей
# остаются вещественными до записи итогового SVG.
brushkernel(radius: real): array of CutCurve
{
    handle := radius*0.5522847498307936;
    north := CutPoint(0.0, -radius);
    east := CutPoint(radius, 0.0);
    south := CutPoint(0.0, radius);
    west := CutPoint(-radius, 0.0);
    return array[] of {
        CutCurve(Cubic, north, CutPoint(handle, -radius), CutPoint(radius, -handle), east),
        CutCurve(Cubic, east, CutPoint(radius, handle), CutPoint(handle, radius), south),
        CutCurve(Cubic, south, CutPoint(-handle, radius), CutPoint(-radius, handle), west),
        CutCurve(Cubic, west, CutPoint(-radius, -handle), CutPoint(-handle, -radius), north),
    };
}


# EN: Find a support point of the borrowed nib in the selected direction.
# One symmetric opposite support supplies the other side of the strip.
# RU: Найти опорную точку отпечатка в заданном направлении. Симметричная
# противоположная точка задаёт вторую сторону полосы.
brushsupport(kernel: array of CutCurve, normal: CutPoint, math: Math): CutPoint
{
    point := kernel[0].a;
    best := cutdot(point, normal);
    for (i := 0; i < len kernel; i++) {
        range := cutextrema(kernel[i], normal, math);
        if (range.hi > best) {
            (candidate, ignored) := cuteval(kernel[i], range.last);
            point = candidate;
            best = range.hi;
        }
    }
    return point;
}


# EN: Construct one exact line in the geometry solver's cubic storage.
# RU: Создать точный отрезок в кубическом представлении решателя.
brushline(a, b: CutPoint): CutCurve
{
    return CutCurve(Segment, a, cutlerp(a, b, 1.0/3.0), cutlerp(a, b, 2.0/3.0), b);
}


# EN: Store one clockwise ring of four spans, enforcing the sweep budget.
# RU: Записать один контур из четырёх участков с проверкой бюджета.
brushring(buffer: ref BrushVector, ring: array of CutCurve): int
{
    base := buffer.count;
    if (base+len ring > len buffer.edges) {
        return 0;
    }
    buffer.edges[base:] = ring;
    for (i := 0; i < len ring; i++) {
        buffer.next[base+i] = base+(i+1)%len ring;
    }
    buffer.count += len ring;
    return 1;
}


# EN: Translate the same four source arcs to an unrounded page position.
# RU: Перенести те же четыре дуги в неокруглённую позицию листа.
brushcircle(kernel: array of CutCurve, position: Point): array of CutCurve
{
    center := cutpoint(position.x, position.y);
    ring := array[len kernel] of CutCurve;
    for (i := 0; i < len ring; i++) {
        edge := kernel[i];
        ring[i] = CutCurve(Cubic, cutoffset(center, edge.a, 1.0), cutoffset(center, edge.b, 1.0),
            cutoffset(center, edge.c, 1.0), cutoffset(center, edge.d, 1.0));
    }
    return ring;
}


# EN: A clockwise strip connects the corresponding support positions.
# Its end chords are covered by the two complete translated nibs.
# RU: Полоса соединяет соответствующие опорные точки по часовой стрелке.
# Её концевые хорды закрывают два полных перенесённых отпечатка.
brushstrip(interval: BrushInterval, kernel: array of CutCurve, math: Math): array of CutCurve
{
    first := cutpoint(interval.first.x, interval.first.y);
    last := cutpoint(interval.last.x, interval.last.y);
    along := cutsub(last, first);
    side := brushsupport(kernel, CutPoint(along.y, -along.x), math);
    a := cutoffset(first, side, 1.0);
    b := cutoffset(last, side, 1.0);
    c := cutoffset(last, side, -1.0);
    d := cutoffset(first, side, -1.0);
    return array[] of {brushline(a, b), brushline(b, c), brushline(c, d), brushline(d, a)};
}


# EN: Store one full stamp for each distinct contributing endpoint.
# RU: Записать полный отпечаток для каждого отдельного конца.
brushends(buffer: ref BrushVector, intervals: array of BrushInterval, kernel: array of CutCurve, work: ref Work): int
{
    points := array[2*len intervals] of Point;
    count := 0;
    for (i := 0; i < len intervals; i++) {
        points[count++] = intervals[i].first;
        points[count++] = intervals[i].last;
    }
    for (i = 0; i < count; i++) {
        checkwork(work);
        duplicate := 0;
        for (j := 0; j < i; j++) {
            if (brushequal(points[i], points[j])) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate && !brushring(buffer, brushcircle(kernel, points[i]))) {
            return 0;
        }
    }
    return 1;
}


# EN: Construct the actual SVG page rectangle as a second, simple outline.
# RU: Построить прямоугольник страницы SVG как второй простой контур.
brushpage(): ref CutOutline
{
    a := CutPoint(0.0, 0.0);
    b := CutPoint(real GridWidth, 0.0);
    c := CutPoint(real GridWidth, real GridHeight);
    d := CutPoint(0.0, real GridHeight);
    edges := array[] of {brushline(a, b), brushline(b, c), brushline(c, d), brushline(d, a)};
    return ref CutOutline(edges, array[] of {1, 2, 3, 0}, array[4] of {* => 0}, 0, nil);
}


# EN: Union a bounded set of fixed-nib strokes, optionally intersecting the
# page in the same real-coordinate arrangement. Only the final curves round.
# RU: Объединить ограниченный набор проходов одного отпечатка. При необходимости
# пересечь со страницей в том же вещественном графе, округлив только результат.
brushunion(path: array of Point, width, clip: int, math: Math, work: ref Work): array of Node
{
    intervals := brushintervals(path, work);
    if (len intervals == 0) {
        return nil;
    }
    kernel := brushkernel(real width*0.5);
    capacity := 12*len intervals;
    if (capacity > CutSpanLimit-4*clip) {
        capacity = CutSpanLimit-4*clip;
    }
    buffer := ref BrushVector(array[capacity] of CutCurve, array[capacity] of int, 0);
    if (!brushends(buffer, intervals, kernel, work)) {
        return nil;
    }
    for (i := 0; i < len intervals; i++) {
        checkwork(work);
        if (!brushring(buffer, brushstrip(intervals[i], kernel, math))) {
            return nil;
        }
    }
    count := buffer.count;
    swept := ref CutOutline(buffer.edges[0:count], buffer.next[0:count], array[count] of {* => 0}, 1, nil);
    sources := CutSources(array[] of {swept}, array[] of {0}, count);
    operation := CutUnion;
    if (clip) {
        sources = CutSources(array[] of {swept, brushpage()}, array[] of {0, count}, count+4);
        operation = CutIntersect;
    }
    contacts := cutcontactbudget(sources.count, work, math);
    result := cutcompose(sources, contacts, operation);
    if (result == nil) {
        return nil;
    }
    return result.nodes;
}

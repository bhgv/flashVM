# Copyright (c) 2026 bhgv
# EN: Compose retained Bézier intervals into editable boundaries.
# RU: Собирать сохранённые участки Безье в редактируемые контуры.


# EN: Private directed graph with exactly one predecessor and successor
# at each retained vertex. Intersection IDs join the two source outlines.
# RU: Частный граф с одним входом и выходом в каждой вершине результата.
# Номера пересечений соединяют оба исходных контура.
CutGraph: adt
{
    points: array of CutPoint; # Endpoint of each source or intersection vertex.
    next: array of int;        # Successor ID or -1 if no retained outgoing edge.
    previous: array of int;    # Predecessor ID or -1.
    spans: array of CutCurve;  # Outgoing curve when next is present.
    modes: array of int;       # Inherited mode per source vertex. Crossings are cusps.
    whole: array of int;       # True where the outgoing source span was not shortened.
};


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


# EN: Classify the midpoint against every filled source except its own.
# RU: Классифицировать середину относительно всех заливок, кроме исходной.
cutinside(outlines: array of ref CutOutline, owner: int, edge: CutCurve, lo, hi: real): int
{
    (p, ignored) := cuteval(edge, (hi+lo)*0.5);
    for (i := 0; i < len outlines; i++) {
        if (i != owner && cutwinding(outlines[i].edges, p) != 0) {
            return 1;
        }
    }
    return 0;
}


# EN: Insert one directed subcurve in the owned graph. Refuse branch points
# or edges whose endpoints collapse at public integer coordinate resolution.
# RU: Вставить направленный участок. Разветвления и совпадение округлённых
# концов отклоняются.
cutinsert(graph: ref CutGraph, edge: CutCurve, from, destination: int, lo, hi: real, reverse: int): int
{
    if (reverse) {
        (from, destination) = (destination, from);
    }
    a := graph.points[from];
    b := graph.points[destination];
    if (graph.next[from] >= 0 || graph.previous[destination] >= 0 ||
        (int a.x == int b.x && int a.y == int b.y)) {
        return 0;
    }
    clipped := cutslice(edge, lo, hi);
    if (reverse) {
        clipped = cutreverse(clipped);
    }
    graph.spans[from] = clipped;
    graph.next[from] = destination;
    graph.previous[destination] = from;
    graph.whole[from] = lo == 0.0 && hi == 1.0;
    return 1;
}


# EN: Find the next crossing on edge after lo. Return (-1, 1.0) at its end.
# RU: Найти следующее пересечение на сегменте после lo. В конце вернуть
# (-1, 1.0).
cutnext(hits: array of CutHit, edge: int, lo: real): (int, real)
{
    at := -1;
    hi := 1.0;
    for (i := 0; i < len hits; i++) {
        hit := hits[i];
        index := hit.second;
        t := hit.u;
        if (hit.first == edge) {
            index = hit.first;
            t = hit.t;
        }
        if (index == edge && t > lo && t < hi) {
            (at, hi) = (i, t);
        }
    }
    return (at, hi);
}


# EN: Retain outer spans for a union or first-minus-second operation. In the
# difference, second's inside spans reverse direction to bound the cut.
# RU: Сохранить внешние участки для слияния или разности. Для разности
# внутренние участки второго контура обращаются и окаймляют вырез.
cutkeep(graph: ref CutGraph, outlines: array of ref CutOutline, hits: array of CutHit,
    shift, hitbase, owner, unite: int, work: ref Work): int
{
    source := outlines[owner];
    edges := source.edges;
    reverse := owner != 0 && !unite;
    for (i := 0; i < len edges; i++) {
        checkwork(work);
        lo := 0.0;
        from := shift+i;
        while (lo < 1.0) {
            (hit, hi) := cutnext(hits, shift+i, lo);
            destination := shift+source.next[i];
            if (hit >= 0) {
                destination = hitbase+hit;
            }
            inside := cutinside(outlines, owner, edges[i], lo, hi);
            if (inside == reverse && !cutinsert(graph, edges[i], from, destination, lo, hi, reverse)) {
                return 0;
            }
            lo = hi;
            from = destination;
        }
    }
    return 1;
}


# EN: Retain an inherited join mode. Shortening either neighbouring handle
# can break symmetry while preserving the tangent at the surviving anchor.
# RU: Сохранить исходный тип стыка. Укороченная ручка может нарушить
# симметрию, сохраняя касательную в уцелевшем узле.
cutjoinmode(graph: ref CutGraph, id: int): int
{
    mode := graph.modes[id];
    if (mode == 2 && (!graph.whole[id] || !graph.whole[graph.previous[id]])) {
        return 1;
    }
    return mode;
}


# EN: Walk directed loops and serialize their original line/Q/C fragments.
# A broken cycle, unrepresentable handle or node limit rejects the operation.
# RU: Обойти циклы и записать исходные фрагменты L/Q/C. Разорванный обход,
# недопустимая ручка или превышение лимита отклоняют операцию.
cutloops(graph: ref CutGraph, work: ref Work): ref CutResult
{
    seen := array[len graph.next] of {* => 0};
    nodes := array[3*len graph.next] of Node;
    modes := array[len nodes] of {* => 0};
    styled := 0;
    for (i := 0; i < len graph.next; i++) {
        checkwork(work);
        if ((graph.next[i] < 0) != (graph.previous[i] < 0)) {
            return nil;
        }
        if (graph.next[i] < 0) {
            continue;
        }
        p := graph.points[i];
        for (j := 0; j < i; j++) {
            q := graph.points[j];
            if (graph.next[j] >= 0 && int p.x == int q.x && int p.y == int q.y) {
                return nil;
            }
        }
    }
    used := 0;
    for (i = 0; i < len graph.next; i++) {
        checkwork(work);
        if (graph.next[i] < 0 || seen[i]) {
            continue;
        }
        base := used;
        start := graph.points[i];
        modes[used] = cutjoinmode(graph, i);
        styled |= modes[used] != 0;
        nodes[used++] = Node(Move, int start.x, int start.y, 0, 0, 0, 0);
        here := i;
        edges := 0;
        while (here >= 0 && !seen[here]) {
            destination := graph.next[here];
            if (destination < 0) {
                return nil;
            }
            node := cutnode(graph.spans[here], graph.points[destination]);
            if (!pointok(node.x, node.y) || !pointok(node.cx, node.cy) || !pointok(node.c2x, node.c2y)) {
                return nil;
            }
            modes[used] = cutjoinmode(graph, destination);
            styled |= modes[used] != 0;
            nodes[used++] = node;
            seen[here] = 1;
            here = destination;
            edges++;
        }
        if (here != i || edges < 2) {
            return nil;
        }
        nodes[used++] = Node(Close, 0, 0, 0, 0, 0, 0);
        (area, ignored) := paintmeasure(nodes[base:used], Point(0, 0), work);
        if (area == big 0) {
            return nil;
        }
    }
    if (used == 0 || used > MaxNodes) {
        return nil;
    }
    if (styled) {
        return ref CutResult(nodes[0:used], modes[0:used]);
    }
    return ref CutResult(nodes[0:used], nil);
}


# EN: Find all borrowed-outline crossings under one shared operation budget.
# Noncrossing nesting and disjoint boundaries have no hit and are valid.
# RU: Найти пересечения контуров в общем бюджете. Допустимы вложенные и
# раздельные границы без пересечений.
cutcrossings(outlines: array of ref CutOutline, offsets: array of int, search: ref CutSearch): int
{
    for (i := 0; i < len outlines; i++) {
        a := outlines[i].edges;
        for (j := i+1; j < len outlines; j++) {
            b := outlines[j].edges;
            for (left := 0; left < len a; left++) {
                for (right := 0; right < len b; right++) {
                    cutpair(a[left], b[right], offsets[i]+left, offsets[j]+right, search);
                    if (search.count < 0) {
                        return 0;
                    }
                }
            }
        }
    }
    return search.count % 2 == 0;
}


# EN: Unite borrowed filled paths or subtract the second from the first.
# A compound input can contain holes and disconnected islands. Resolve all
# crossings before the single integer serialization. Nil defers to coverage.
# RU: Объединить заливки или вычесть вторую из первой. Во входном пути
# допустимы отверстия и островки. Все пересечения вычисляются до одного
# округления координат. Nil передаёт работу обработке по покрытию.
cutboolean(paths: array of CutResult, unite: int, work: ref Work): ref CutResult
{
    if (len paths < 2 || len paths > 16 || (!unite && len paths != 2)) {
        return nil;
    }
    search := ref CutSearch(array[96] of CutHit, 0, 40000, work);
    outlines := array[len paths] of ref CutOutline;
    offsets := array[len paths] of int;
    hitbase := 0;
    for (i := 0; i < len paths; i++) {
        outline := cutoutline(paths[i], search);
        if (outline == nil || hitbase+len outline.edges > 192) {
            return nil;
        }
        outlines[i] = outline;
        offsets[i] = hitbase;
        hitbase += len outline.edges;
    }
    if (!cutcrossings(outlines, offsets, search)) {
        return nil;
    }
    size := hitbase+search.count;
    graph := ref CutGraph(array[size] of CutPoint, array[size] of {* => -1},
        array[size] of {* => -1}, array[size] of CutCurve, array[size] of {* => 0}, array[size] of int);
    for (i = 0; i < len outlines; i++) {
        source := outlines[i];
        shift := offsets[i];
        for (j := 0; j < len source.edges; j++) {
            graph.points[shift+j] = source.edges[j].a;
        }
        graph.modes[shift:] = source.modes;
    }
    for (i = 0; i < search.count; i++) {
        graph.points[hitbase+i] = search.hits[i].at;
    }
    hits := search.hits[0:search.count];
    for (i = 0; i < len outlines; i++) {
        if (!cutkeep(graph, outlines, hits, offsets[i], hitbase, i, unite, work)) {
            return nil;
        }
    }
    return cutloops(graph, work);
}

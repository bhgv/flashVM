# Copyright (c) 2026 bhgv
# EN: Validate and orient compound paths for the private brush boolean.
# RU: Проверять и ориентировать составные пути для операций кисти.


# EN: An owned editable outline and its optional per-anchor join modes.
# RU: Редактируемый контур с необязательными типами стыков по узлам.
CutResult: adt
{
    nodes: array of Node; # Immutable SVG commands.
    modes: array of int;  # Aligned join modes, nil for all cusps.
};


# EN: Boundary edges of one filled path. Every edge has fill on its right
# in page coordinates. Successors wrap at each individual closed subpath.
# RU: Граница одной заливки. Заливка справа по ходу каждого ребра.
# Следующие индексы замыкаются отдельно для каждого подпути.
CutOutline: adt
{
    edges: array of CutCurve; # Owned oriented edge values.
    next: array of int;       # End vertex ID for each edge.
    modes: array of int;      # Join mode of the start vertex of each edge.
};


# EN: One validated, simple input ring before nonzero winding classification.
# RU: Одно проверенное простое кольцо до классификации ненулевого обхода.
CutRing: adt
{
    edges: array of CutCurve; # Owned segments in original direction.
    first: int;               # First source command of this subpath.
    end: int;                 # Exclusive source command bound.
    sign: int;                # +1 clockwise, -1 counterclockwise.
};


# EN: Return signed winding at a copied point against borrowed segments.
# RU: Вернуть знаковый обход точки заимствованными сегментами.
cutwinding(edges: array of CutCurve, p: CutPoint): int
{
    winding := 0;
    for (i := 0; i < len edges; i++) {
        winding += cutray(edges[i], p, 0);
    }
    return winding;
}


# EN: Reject touching or crossing borrowed rings of the same source.
# Nested rings with disjoint boundaries remain eligible.
# RU: Отклонить пересекающиеся и касающиеся кольца одного источника.
# Вложенные кольца с раздельными границами допустимы.
cutringapart(a, b: array of CutCurve, search: ref CutSearch): int
{
    for (i := 0; i < len a; i++) {
        for (j := 0; j < len b; j++) {
            cutpair(a[i], b[j], i, j, search);
            if (search.count != 0) {
                return 0;
            }
        }
    }
    return 1;
}


# EN: Look up the inherited mode of a source anchor by its unique position
# in a simple ring. At an explicit closing endpoint use either nonzero mode.
# RU: Найти тип исходного стыка по уникальной позиции в простом кольце.
# У явно продублированной точки замыкания берётся ненулевой тип.
cutanchormode(source: CutResult, ring: CutRing, point: CutPoint): int
{
    if (len source.modes == len source.nodes) {
        for (i := ring.first; i < ring.end; i++) {
            node := source.nodes[i];
            mode := source.modes[i];
            if (mode != 0 && node.verb != Close && node.x == int point.x && node.y == int point.y) {
                return mode;
            }
        }
    }
    return 0;
}


# EN: Flatten validated rings under SVG nonzero fill. Test whether crossing
# each boundary changes fill, omit redundant boundaries, and orient the
# surviving ones using their effective inside/outside state.
# RU: Собрать проверенные кольца по правилу ненулевого обхода SVG.
# Лишние границы пропускаются. Остальные ориентируются по состоянию заливки.
cutorient(source: CutResult, rings: array of CutRing, search: ref CutSearch): ref CutOutline
{
    edges := array[len source.nodes] of CutCurve;
    successors := array[len source.nodes] of int;
    modes := array[len source.nodes] of int;
    used := 0;
    for (i := 0; i < len rings; i++) {
        checkwork(search.work);
        ring := rings[i];
        p := ring.edges[0].a;
        outside := 0;
        for (j := 0; j < len rings; j++) {
            if (j != i) {
                outside += cutwinding(rings[j].edges, p);
            }
        }
        inside := outside+ring.sign;
        if ((outside == 0) == (inside == 0)) {
            continue;
        }
        expected := 1;
        if (outside != 0) {
            expected = -1;
        }
        base := used;
        for (j = 0; j < len ring.edges; j++) {
            edge := ring.edges[j];
            if (ring.sign != expected) {
                edge = cutreverse(ring.edges[len ring.edges-1-j]);
            }
            edges[used] = edge;
            successors[used] = base+(j+1)%len ring.edges;
            modes[used] = cutanchormode(source, ring, edge.a);
            used++;
        }
    }
    if (used == 0) {
        return nil;
    }
    return ref CutOutline(edges[0:used], successors[0:used], modes[0:used]);
}


# EN: Accept the borrowed source as a bounded collection of simple,
# non-touching closed subpaths. Nil defers uncertain topology to the mask.
# RU: Принять ограниченный набор простых замкнутых непересекающихся подпутей.
# Nil передаёт неоднозначную топологию обработке по маске.
cutoutline(source: CutResult, search: ref CutSearch): ref CutOutline
{
    nodes := source.nodes;
    if (len nodes < 4 || len nodes > 192) {
        return nil;
    }
    rings := array[16] of CutRing;
    base := 0;
    count := 0;
    for (i := 0; i < len nodes; i++) {
        if (nodes[i].verb != Close) {
            continue;
        }
        if (count == len rings) {
            return nil;
        }
        span := nodes[base:i+1];
        edges := cutcurves(span);
        if (edges == nil || !cutsimple(edges, search)) {
            return nil;
        }
        (area, ignored) := paintmeasure(span, Point(0, 0), search.work);
        if (area == big 0) {
            return nil;
        }
        for (j := 0; j < count; j++) {
            if (!cutringapart(edges, rings[j].edges, search)) {
                return nil;
            }
        }
        sign := 1;
        if (area < big 0) {
            sign = -1;
        }
        rings[count++] = CutRing(edges, base, i+1, sign);
        base = i+1;
    }
    if (base != len nodes || count == 0) {
        return nil;
    }
    return cutorient(source, rings[0:count], search);
}

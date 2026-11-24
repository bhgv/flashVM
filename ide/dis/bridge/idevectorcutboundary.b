# Copyright (c) 2026 bhgv
# EN: Link retained curve fragments around shared contour vertices.
# RU: Связывать сохранённые фрагменты кривых в общих вершинах контуров.


# EN: One oriented boundary interval. A source ID tracks unchanged adjacency
# separately from the canonical geometric endpoints of the fragment.
# RU: Один направленный участок границы. Исходные номера нужны для стыков
# и хранятся отдельно от общих геометрических концов фрагмента.
CutArc: adt
{
    first: int;        # Canonical initial vertex.
    last: int;         # Canonical terminal vertex.
    curve: CutCurve;   # Copied and possibly trimmed directed curve.
    initial: int;      # Original initial anchor ID or -1 at a split.
    terminal: int;     # Original terminal anchor ID or -1 at a split.
    whole: int;        # True if the fragment is an entire source span.
};


# EN: Private directed boundary. Vertices may have several incident arcs
# when closed regions touch at one point. Arc continuations remain unique.
# RU: Частная направленная граница. Касающиеся области могут иметь несколько
# дуг в общей вершине. Продолжение каждой дуги однозначно.
CutBoundary: adt
{
    points: array of CutPoint; # Original vertex and event positions.
    aliases: array of int;     # Earliest index for each exact shared position.
    modes: array of int;       # Join modes indexed by original source IDs.
    arcs: array of CutArc;    # Directed fragments initialized up to count.
    starts: array of int;     # First outgoing arc for each canonical vertex.
    siblings: array of int;   # Next outgoing arc from the same vertex.
    count: int;               # Initialized arc count.
};


# EN: Hash bins for already visited geometric vertices.
# RU: Хеш-ячейки уже обработанных геометрических вершин.
CutPointBins: adt
{
    heads: array of int; # First inserted index for a hashed cell, or -1.
    next: array of int;  # Chain link for each inserted vertex.
};


# EN: Allocate scratch cells with space proportional to the input vertices.
# RU: Выделить временные ячейки пропорционально числу входных вершин.
cutpointbins(count: int): ref CutPointBins
{
    size := 8;
    while (size < 4*count) {
        size *= 2;
    }
    return ref CutPointBins(array[size] of {* => -1}, array[count] of int);
}


# EN: Hash a copied integer cell into a power-of-two table.
# RU: Отобразить целочисленную ячейку в таблицу размера степени двойки.
cutpointcell(bins: ref CutPointBins, x, y: int): int
{
    return ((7919*x)^(104729*y)) & (len bins.heads-1);
}


# EN: Add a source vertex to its rounded-coordinate bucket.
# RU: Добавить исходную вершину в ячейку округлённых координат.
cutpointadd(bins: ref CutPointBins, point: CutPoint, id: int)
{
    cell := cutpointcell(bins, int point.x, int point.y);
    bins.next[id] = bins.heads[cell];
    bins.heads[cell] = id;
}


# EN: Find the earliest previous match. Tolerant queries check neighbouring
# cells because a real coincidence can straddle a rounding boundary.
# RU: Найти первое совпадение. Для вещественного допуска проверяются
# соседние ячейки, поскольку совпадение может пересечь границу округления.
cutpointfind(bins: ref CutPointBins, points: array of CutPoint, id, nearby: int): int
{
    p := points[id];
    x := int p.x;
    y := int p.y;
    best := id;
    for (dy := -nearby; dy <= nearby; dy++) {
        for (dx := -nearby; dx <= nearby; dx++) {
            cell := cutpointcell(bins, x+dx, y+dy);
            for (j := bins.heads[cell]; j >= 0; j = bins.next[j]) {
                q := points[j];
                if (j < best) {
                    if ((nearby && cutsamepoint(p, q)) || (!nearby && x == int q.x && y == int q.y)) {
                        best = j;
                    }
                }
            }
        }
    }
    return best;
}


# EN: Prepare canonical position IDs, leaving actual source directions alone.
# Distinct positions which only meet after rounding remain distinct here.
# RU: Назначить общие номера совпадающим позициям. Точки, совпадающие только
# после округления, пока остаются раздельными.
cutaliases(boundary: ref CutBoundary, work: ref Work)
{
    bins := cutpointbins(len boundary.points);
    for (i := 0; i < len boundary.points; i++) {
        checkwork(work);
        boundary.aliases[i] = i;
        previous := cutpointfind(bins, boundary.points, i, 1);
        boundary.aliases[i] = boundary.aliases[previous];
        cutpointadd(bins, boundary.points[i], i);
    }
}


# EN: Retain one directed interval. Deduplicate repeated views of precisely
# the same seam. Different curves between the same vertices remain separate.
# RU: Сохранить направленный участок и убрать повторы одной кромки.
# Разные кривые между теми же вершинами остаются раздельными.
cutinsert(boundary: ref CutBoundary, edge: CutCurve, from, destination, initial, terminal: int,
    lo, hi: real, reverse: int): int
{
    from = boundary.aliases[from];
    destination = boundary.aliases[destination];
    if (reverse) {
        (from, destination) = (destination, from);
        (initial, terminal) = (terminal, initial);
    }
    a := boundary.points[from];
    b := boundary.points[destination];
    if (int a.x == int b.x && int a.y == int b.y) {
        return 0;
    }
    clipped := cutslice(edge, lo, hi);
    if (reverse) {
        clipped = cutreverse(clipped);
    }
    for (i := boundary.starts[from]; i >= 0; i = boundary.siblings[i]) {
        arc := boundary.arcs[i];
        if (arc.last == destination && (cutsamecurve(arc.curve, clipped) ||
            (cutstraight(arc.curve) && cutstraight(clipped)))) {
            return 1;
        }
    }
    if (boundary.count == len boundary.arcs) {
        return 0;
    }
    at := boundary.count++;
    boundary.arcs[at] = CutArc(from, destination, clipped, initial, terminal, lo == 0.0 && hi == 1.0);
    boundary.siblings[at] = boundary.starts[from];
    boundary.starts[from] = at;
    return 1;
}


# EN: Measure the outgoing ray slightly inside the arc. Near an exact
# tangent this distinguishes the two sides by actual local curvature.
# RU: Измерить исходящий луч чуть внутри дуги. При общей касательной
# такой луч различает стороны по местной кривизне.
cutarcray(edge: CutCurve): CutPoint
{
    (at, ignored) := cuteval(edge, 0.001);
    return cutsub(at, edge.a);
}


# EN: Select the clockwise boundary continuation at a contact by the
# angular order of outgoing rays. Nil alignment is represented by -1.
# RU: Выбрать продолжение границы в точке контакта по порядку лучей.
# Неопределённое направление обозначается -1.
cutfollowing(boundary: ref CutBoundary, arc: CutArc, math: Math): int
{
    first := boundary.starts[arc.last];
    if (first < 0 || boundary.siblings[first] < 0) {
        return first;
    }
    ray := cutarcray(cutreverse(arc.curve));
    chosen := -1;
    best := 7.0;
    for (i := first; i >= 0; i = boundary.siblings[i]) {
        outgoing := cutarcray(boundary.arcs[i].curve);
        cross := cutcross(outgoing, ray);
        dot := cutdot(outgoing, ray);
        if (cross == 0.0 && dot > 0.0) {
            return -1;
        }
        angle := math->atan2(cross, dot);
        if (angle < 0.0) {
            angle += 6.283185307179586;
        }
        if (angle > best-1.0e-8 && angle < best+1.0e-8) {
            return -1;
        }
        if (angle < best) {
            chosen = i;
            best = angle;
        }
    }
    return chosen;
}


# EN: Validate the balanced outgoing relation and give every retained arc
# exactly one predecessor. Distinct points that collide on the output grid
# are refused even if their unrounded paths do not touch.
# RU: Проверить баланс и единственного предшественника для каждой дуги.
# Разные точки, совпавшие только на сетке результата, отклоняются.
cutlinks(boundary: ref CutBoundary, math: Math, work: ref Work): array of int
{
    bins := cutpointbins(len boundary.points);
    for (i := 0; i < len boundary.points; i++) {
        checkwork(work);
        if (boundary.starts[i] < 0) {
            continue;
        }
        if (cutpointfind(bins, boundary.points, i, 0) != i) {
            return nil;
        }
        cutpointadd(bins, boundary.points[i], i);
    }
    following := array[boundary.count] of int;
    predecessor := array[boundary.count] of {* => 0};
    for (i = 0; i < boundary.count; i++) {
        checkwork(work);
        next := cutfollowing(boundary, boundary.arcs[i], math);
        if (next < 0 || predecessor[next]) {
            return nil;
        }
        following[i] = next;
        predecessor[next] = 1;
    }
    return following;
}


# EN: Preserve the join only when the two fragments meet at the same
# original anchor. Crossing to another source produces a cusp.
# RU: Сохранить стык только при встрече фрагментов в одном исходном узле.
# Переход на другой источник создаёт угловой узел.
cutjoinmode(boundary: ref CutBoundary, before, after: CutArc): int
{
    if (before.terminal < 0 || before.terminal != after.initial) {
        return 0;
    }
    mode := boundary.modes[before.terminal];
    if (mode == 2 && (!before.whole || !after.whole)) {
        return 1;
    }
    return mode;
}


# EN: Walk every directed cycle in the completed relation. Regions meeting
# at a vertex serialize as separate closed paths when their fill faces differ.
# RU: Обойти направленные циклы. Области, встретившиеся в вершине,
# записываются отдельными путями, если это разные стороны заливки.
cutloops(boundary: ref CutBoundary, math: Math, work: ref Work): ref CutResult
{
    if (boundary.count == 0) {
        return nil;
    }
    following := cutlinks(boundary, math, work);
    if (following == nil) {
        return nil;
    }
    seen := array[boundary.count] of {* => 0};
    nodes := array[3*boundary.count] of Node;
    modes := array[len nodes] of {* => 0};
    styled := 0;
    used := 0;
    for (i := 0; i < boundary.count; i++) {
        checkwork(work);
        if (seen[i]) {
            continue;
        }
        base := used;
        first := boundary.arcs[i];
        start := boundary.points[first.first];
        nodes[used++] = Node(Move, int start.x, int start.y, 0, 0, 0, 0);
        here := i;
        edges := 0;
        while (!seen[here]) {
            arc := boundary.arcs[here];
            next := following[here];
            node := cutnode(arc.curve, boundary.points[arc.last]);
            if (!pointok(node.x, node.y) || !pointok(node.cx, node.cy) || !pointok(node.c2x, node.c2y)) {
                return nil;
            }
            mode := cutjoinmode(boundary, arc, boundary.arcs[next]);
            modes[used] = mode;
            styled |= mode != 0;
            nodes[used++] = node;
            seen[here] = 1;
            here = next;
            edges++;
        }
        if (here != i || edges < 2) {
            return nil;
        }
        modes[base] = modes[used-1];
        nodes[used++] = Node(Close, 0, 0, 0, 0, 0, 0);
        (area, ignored) := paintmeasure(nodes[base:used], Point(0, 0), work);
        if (area == big 0) {
            return nil;
        }
    }
    if (used > MaxNodes) {
        return nil;
    }
    if (styled) {
        return ref CutResult(nodes[0:used], modes[0:used]);
    }
    return ref CutResult(nodes[0:used], nil);
}

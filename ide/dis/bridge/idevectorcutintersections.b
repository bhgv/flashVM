# Copyright (c) 2026 bhgv
# EN: Isolate transverse Bézier crossings with bounded hull subdivision.
# RU: Изолировать поперечные пересечения Безье ограниченным делением оболочек.


# EN: A shared crossing and its parameter on each original contour edge.
# RU: Общее пересечение и его параметр на каждом исходном сегменте.
CutHit: adt
{
    first: int;   # Edge index in the first outline.
    second: int;  # Edge index in the second outline.
    t: real;      # Parameter on first.
    u: real;      # Parameter on second.
    at: CutPoint; # Shared position before SVG rounding.
};


# EN: Private worker scratch for one boolean. A negative count declines it.
# RU: Локальные данные воркера. Отрицательный count отклоняет операцию.
CutSearch: adt
{
    hits: array of CutHit; # Owned, bounded candidate list.
    count: int;           # Initialized crossings, or -1 on ambiguity.
    budget: int;          # Remaining hull visits across the operation.
    work: ref Work;       # Borrowed cooperative cancellation state.
};


# EN: One original pair and its contour IDs. Copies survive leaf subdivision.
# RU: Исходная пара и номера сегментов, сохраняемые при делении.
CutPair: adt
{
    first: int;  # First contour edge index.
    second: int; # Second contour edge index.
    a: CutCurve; # Unsubdivided first edge.
    b: CutCurve; # Unsubdivided second edge.
};


# EN: One parameter interval and its hull with local control points.
# RU: Интервал параметра и оболочка с локальными управляющими точками.
CutInterval: adt
{
    edge: CutCurve; # Local cubic over [lo, hi].
    lo: real;      # First parameter on the original segment.
    hi: real;      # Last parameter on the original segment.
};


# EN: Store one shared crossing or contact, deduplicating repeated leaves
# of the same original pair. Coincident span endpoints use the same table.
# RU: Записать пересечение или касание, удаляя повторы одной пары сегментов.
# Концы совпавших участков используют ту же таблицу.
cuthitadd(search: ref CutSearch, pair: CutPair, t, u: real, point: CutPoint)
{
    for (i := 0; i < search.count; i++) {
        hit := search.hits[i];
        dt := t-hit.t;
        du := u-hit.u;
        if (hit.first == pair.first && hit.second == pair.second && dt*dt+du*du < 1.0e-12) {
            return;
        }
    }
    if (search.count == len search.hits) {
        search.count = -1;
        return;
    }
    search.hits[search.count++] = CutHit(pair.first, pair.second, t, u, point);
}


# EN: Refine one isolated root, rejecting unresolved overlap and tangencies.
# Intersections near original vertices use the coverage path. Multiple leaves
# can converge to the same root, which is stored only once.
# RU: Уточнить один корень. Неопределённые наложения, касания и попадания
# в исходные вершины идут по маске. Повторные корни сохраняются один раз.
cutroot(pair: CutPair, a, b: CutInterval, search: ref CutSearch)
{
    t := (a.lo+a.hi)*0.5;
    u := (b.lo+b.hi)*0.5;
    for (i := 0; i < 10; i++) {
        (p, dp) := cuteval(pair.a, t);
        (q, dq) := cuteval(pair.b, u);
        delta := cutsub(p, q);
        cross := cutcross(dp, dq);
        if (cross*cross <= 1.0e-10*cutdot(dp, dp)*cutdot(dq, dq)) {
            break;
        }
        if (cutdot(delta, delta) < 1.0e-12) {
            if (t < a.lo-0.000001 || t > a.hi+0.000001 || u < b.lo-0.000001 || u > b.hi+0.000001) {
                return;
            }
            if (t <= 0.00001 || t >= 0.99999 || u <= 0.00001 || u >= 0.99999) {
                break;
            }
            cuthitadd(search, pair, t, u, cutlerp(p, q, 0.5));
            return;
        }
        t -= cutcross(delta, dq)/cross;
        u -= cutcross(delta, dp)/cross;
        if (t < -0.01 || t > 1.01 || u < -0.01 || u > 1.01) {
            break;
        }
    }
    search.count = -1;
}


# EN: Find all crossings between borrowed subintervals by rejecting separated
# control hulls. Bisect the larger hull to a subpixel box before Newton.
# RU: Найти пересечения заимствованных участков, отсекая раздельные оболочки.
# Большая оболочка делится до субпиксельного размера перед методом Ньютона.
cutsearch(pair: CutPair, a, b: CutInterval, search: ref CutSearch, depth: int)
{
    if (search.count < 0) {
        return;
    }
    if (search.budget % 64 == 0) {
        checkwork(search.work);
    }
    search.budget--;
    if (search.budget < 0) {
        search.count = -1;
        return;
    }
    ab := cutbounds(a.edge);
    bb := cutbounds(b.edge);
    if (cutapart(ab, bb)) {
        return;
    }
    av := cutextent(ab);
    bv := cutextent(bb);
    if ((av <= 0.0625 && bv <= 0.0625) || depth == 40) {
        cutroot(pair, a, b, search);
        return;
    }
    if (av >= bv) {
        (left, right) := cuthalf(a.edge);
        t := (a.lo+a.hi)*0.5;
        cutsearch(pair, CutInterval(left, a.lo, t), b, search, depth+1);
        cutsearch(pair, CutInterval(right, t, a.hi), b, search, depth+1);
    } else {
        (left, right) := cuthalf(b.edge);
        u := (b.lo+b.hi)*0.5;
        cutsearch(pair, a, CutInterval(left, b.lo, u), search, depth+1);
        cutsearch(pair, a, CutInterval(right, u, b.hi), search, depth+1);
    }
}


# EN: Inspect one immutable pair of complete edges with original edge indices.
# RU: Проверить одну неизменяемую пару целых сегментов с исходными номерами.
cutpair(a, b: CutCurve, i, j: int, search: ref CutSearch)
{
    cutsearch(CutPair(i, j, a, b), CutInterval(a, 0.0, 1.0), CutInterval(b, 0.0, 1.0), search, 0);
}


# EN: Check whether a contour is eligible for the simple-path boolean.
# Projection monotonicity excludes a loop within one cubic. Trim the shared
# endpoints of adjacent edges before testing for a crossing elsewhere.
# RU: Проверить пригодность контура. Проекция исключает петлю внутри одной
# кривой. Общие концы соседей отрезаются перед поиском других пересечений.
cutsimple(edges: array of CutCurve, search: ref CutSearch): int
{
    for (i := 0; i < len edges; i++) {
        a := edges[i];
        direction := cutsub(a.d, a.a);
        if (cutdot(cutsub(a.b, a.a), direction) < 0.0 ||
            cutdot(cutsub(a.c, a.b), direction) < 0.0 ||
            cutdot(cutsub(a.d, a.c), direction) < 0.0) {
            return 0;
        }
        for (j := i+1; j < len edges; j++) {
            first := a;
            second := edges[j];
            if (j == i+1) {
                first = cutslice(first, 0.0, 0.9999);
                second = cutslice(second, 0.0001, 1.0);
            }
            if (i == 0 && j == len edges-1) {
                first = cutslice(first, 0.0001, 1.0);
                second = cutslice(second, 0.0, 0.9999);
            }
            cutpair(first, second, i, j, search);
            if (search.count != 0) {
                return 0;
            }
        }
    }
    return 1;
}

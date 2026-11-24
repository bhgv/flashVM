# Copyright (c) 2026 bhgv
# EN: Index stroke intersections by cached bounds and incident source edge.
# RU: Индексировать пересечения мазка по сохранённым границам и исходным рёбрам.


# EN: One original edge in the sweep of cached control hulls.
# RU: Одно исходное ребро в проходе по управляющим оболочкам.
CutEdgeKey: adt
{
    box: CutBox;     # Conservative Bézier control bounds in page coordinates.
    owner: int;     # Outline index.
    index: int;     # Local edge index in that outline.
    id: int;        # Global geometric and provenance index.
    first: real;    # Near end of the selected sweep projection.
    last: real;     # Far end of the selected sweep projection.
};


# EN: Per-edge linked lists of event sides. Slot 2*k is the first side
# of event k, and slot 2*k+1 is its second side.
# RU: Списки сторон событий для каждого ребра. Ячейка 2*k хранит первую
# сторону события k, а ячейка 2*k+1 — вторую.
CutChain: adt
{
    heads: array of int; # First slot for each global edge, -1 when empty.
    next: array of int;  # Next slot for the same edge.
};


# EN: Allocate one private incident-event index with explicit empty heads.
# RU: Выделить частный индекс событий с пустыми головами списков.
cutchain(edges, events: int): ref CutChain
{
    return ref CutChain(array[edges] of {* => -1}, array[events*2] of int);
}


# EN: Link an owned event side into its edge's list.
# RU: Добавить сторону события в список её ребра.
cutchainadd(chain: ref CutChain, edge, slot: int)
{
    chain.next[slot] = chain.heads[edge];
    chain.heads[edge] = slot;
}


# EN: Build indexes once after discovery. Insert backwards to keep the
# original event order on each individual edge.
# RU: Построить индексы после поиска. Обратная вставка сохраняет порядок
# событий на каждом отдельном ребре.
cutincidence(contacts: ref CutContacts, edges: int): ref CutChain
{
    search := contacts.search;
    hits := cutchain(edges, search.count);
    for (i := search.count-1; i >= 0; i--) {
        hit := search.hits[i];
        cutchainadd(hits, hit.first, 2*i);
        cutchainadd(hits, hit.second, 2*i+1);
    }
    seams := cutchain(edges, contacts.count);
    for (i = contacts.count-1; i >= 0; i--) {
        seam := contacts.seams[i];
        cutchainadd(seams, seam.first, 2*i);
        cutchainadd(seams, seam.second, 2*i+1);
    }
    contacts.index = seams;
    return hits;
}


# EN: Store a copied union of two conservative bounds.
# RU: Вернуть общие консервативные границы двух прямоугольников.
cutboxunion(a, b: CutBox): CutBox
{
    if (b.left < a.left) {
        a.left = b.left;
    }
    if (b.top < a.top) {
        a.top = b.top;
    }
    if (b.right > a.right) {
        a.right = b.right;
    }
    if (b.bottom > a.bottom) {
        a.bottom = b.bottom;
    }
    return a;
}


# EN: Cache curve bounds and select the less crowded projection. Normalizing
# by the source extent avoids preferring a short but densely stacked axis.
# RU: Сохранить границы и выбрать менее загруженную проекцию. Деление на
# размах источника учитывает плотность короткой оси.
cutedgekeys(sources: CutSources, work: ref Work): array of CutEdgeKey
{
    keys := array[sources.count] of CutEdgeKey;
    extent := cutbounds(sources.outlines[0].edges[0]);
    width := 0.0;
    height := 0.0;
    for (i := 0; i < len sources.outlines; i++) {
        edges := sources.outlines[i].edges;
        offset := sources.offsets[i];
        for (j := 0; j < len edges; j++) {
            checkwork(work);
            box := cutbounds(edges[j]);
            extent = cutboxunion(extent, box);
            width += box.right-box.left;
            height += box.bottom-box.top;
            keys[offset+j] = CutEdgeKey(box, i, j, offset+j, 0.0, 0.0);
        }
    }
    horizontal := width*(extent.bottom-extent.top) <= height*(extent.right-extent.left);
    for (i = 0; i < len keys; i++) {
        box := keys[i].box;
        if (horizontal) {
            (keys[i].first, keys[i].last) = (box.left, box.right);
        } else {
            (keys[i].first, keys[i].last) = (box.top, box.bottom);
        }
    }
    return keys;
}


# EN: Sort copied keys stably with a bottom-up merge, so equal bounds keep
# their original source order without a quadratic insertion pass.
# RU: Устойчиво отсортировать границы слиянием снизу вверх. Равные
# границы сохраняют исходный порядок без квадратичной вставки.
cutedgesort(keys: array of CutEdgeKey, work: ref Work): array of CutEdgeKey
{
    other := array[len keys] of CutEdgeKey;
    count := len keys;
    for (width := 1; width < count; width *= 2) {
        for (start := 0; start < count; start += 2*width) {
            checkwork(work);
            middle := start+width;
            finish := middle+width;
            if (middle > count) {
                middle = count;
            }
            if (finish > count) {
                finish = count;
            }
            a := start;
            b := middle;
            for (out := start; out < finish; out++) {
                if (b == finish || (a < middle && keys[a].first <= keys[b].first)) {
                    other[out] = keys[a++];
                } else {
                    other[out] = keys[b++];
                }
            }
        }
        (keys, other) = (other, keys);
    }
    return keys;
}


# EN: Record contacts for one eligible bounding pair, keeping original ID
# order. Suppress a natural adjacent endpoint within the same outline.
# RU: Записать контакты допустимой пары в порядке исходных номеров.
# Обычный общий конец соседних рёбер внутри контура пропускается.
cutrecord(sources: CutSources, a, b: CutEdgeKey, contacts: ref CutContacts): int
{
    if (a.id > b.id) {
        (a, b) = (b, a);
    }
    first := sources.outlines[a.owner];
    second := sources.outlines[b.owner];
    if (a.owner == b.owner && !first.internal) {
        return 1;
    }
    search := contacts.search;
    base := search.count;
    cutcontactpair(CutPair(a.id, b.id, first.edges[a.index], second.edges[b.index], 1), contacts);
    if (search.count < 0) {
        return 0;
    }
    if (a.owner == b.owner) {
        kept := base;
        for (i := base; i < search.count; i++) {
            hit := search.hits[i];
            forward := first.next[a.index] == b.index && hit.t >= 1.0-1.0e-9 && hit.u <= 1.0e-9;
            backward := first.next[b.index] == a.index && hit.u >= 1.0-1.0e-9 && hit.t <= 1.0e-9;
            if (!forward && !backward) {
                search.hits[kept++] = hit;
            }
        }
        search.count = kept;
    }
    return 1;
}


# EN: Sweep the cached intervals. A separate comparison budget covers
# crowded bounding boxes even when none of them intersects in both axes.
# RU: Пройти сохранённые интервалы. Отдельный бюджет ограничивает плотные
# оболочки, даже когда пересечений по двум осям нет.
cutbroadphase(sources: CutSources, contacts: ref CutContacts): int
{
    work := contacts.search.work;
    keys := cutedgesort(cutedgekeys(sources, work), work);
    budget := 8192+450*len keys;
    for (i := 0; i < len keys; i++) {
        checkwork(work);
        a := keys[i];
        for (j := i+1; j < len keys && keys[j].first <= a.last; j++) {
            budget--;
            if (budget <= 0) {
                return 0;
            }
            if (budget % 128 == 0) {
                checkwork(work);
            }
            b := keys[j];
            if (!cutapart(a.box, b.box) && !cutrecord(sources, a, b, contacts)) {
                return 0;
            }
        }
    }
    return 1;
}

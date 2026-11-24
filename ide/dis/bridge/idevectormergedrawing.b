# Copyright (c) 2026 bhgv
# EN: Apply Merge Drawing's opaque colour partition to one released brush.
# RU: Применять разбиение непрозрачных заливок Merge Drawing к отпущенной кисти.


# EN: A borrowed immutable mask is empty when there are no covered cells.
# RU: Заимствованная маска пуста, когда ни одна ячейка не покрыта.
paintempty(mask: array of byte, work: ref Work): int
{
    for (i := 0; i < len mask; i++) {
        if (i % 512 == 0) {
            checkwork(work);
        }
        if (mask[i] != byte 0) {
            return 0;
        }
    }
    return 1;
}


# EN: Return a new remainder if any source cells intersect cut; nil means
# the caller may keep source and its original vector geometry.
# RU: Вернуть новый остаток при пересечении source и cut. Nil позволяет
# сохранить исходную маску и её векторную геометрию.
paintsubtract(source, cut: array of byte, work: ref Work): array of byte
{
    remainder: array of byte;
    for (i := 0; i < len source; i++) {
        if (i % 512 == 0) {
            checkwork(work);
        }
        if ((source[i] & cut[i]) != byte 0) {
            if (remainder == nil) {
                remainder = array[len source] of byte;
                remainder[0:] = source;
            }
            remainder[i] &= (byte 255 ^ cut[i]);
        }
    }
    return remainder;
}


# EN: Compare immutable coverage; nil masks are unavailable, never equal.
# RU: Сверить неизменяемое покрытие. Nil означает, что маска недоступна.
paintsame(a, b: array of byte, work: ref Work): int
{
    if (a == nil || b == nil) {
        return 0;
    }
    for (i := 0; i < len a; i++) {
        if (i % 512 == 0) {
            checkwork(work);
        }
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}


# EN: Compute visible same-colour candidates from top to bottom. Keep a borrowed
# or newly rendered mask for each entire shape, including covered parts, to
# remove the new footprint from lower shapes without revealing buried paint.
# RU: Сверху вниз вычислить видимые одноцветные области. Сохранить маску полной
# фигуры, включая скрытые участки, чтобы при переносе мазка они не открывались.
paintlayers(doc: ref Document, color, count: int): (array of array of byte, array of array of byte)
{
    above := zerobytes(GridBytes);
    areas := array[count] of array of byte;
    visible := array[count] of array of byte;
    for (index := count - 1; index >= 0; index--) {
        old := doc.shapes[index];
        mask := shapearea(old, doc.work);
        areas[index] = mask;
        if (old.color == color && old.fill) {
            visible[index] = array[GridBytes] of byte;
        }
        for (part := 0; part < GridBytes; part++) {
            if (part % 512 == 0) {
                checkwork(doc.work);
            }
            if (visible[index] != nil) {
                visible[index][part] = mask[part] & (byte 255 ^ above[part]);
            }
            above[part] |= mask[part];
        }
    }
    return (areas, visible);
}


# EN: Apply one stroke to a private document. Only publish complete bounded
# geometry. The original shapes and their masks belong to immutable Undo.
# RU: Применить один мазок к частному документу. Публиковать только полный
# допустимый результат. Исходные фигуры и маски принадлежат истории Undo.
mergebrush(doc: ref Document, raw: ref Shape): int
{
    exact := brushcapsule(raw, doc.work);
    if (raw.fill && raw.width == 0) {
        exact = raw.nodes;
    }
    footprint: array of byte;
    if (exact != nil) {
        footprint = shapearea(ref Shape(raw.color, 0, 1, exact, nil, 0, nil), doc.work);
    } else {
        footprint = shapearea(raw, doc.work);
    }
    count := len doc.shapes - 1;
    (areas, visible) := paintlayers(doc, raw.color, count);
    used := array[count] of { * => 0 };
    combined := array[GridBytes] of byte;
    combined[0:] = footprint;
    merged := 0;
    changed := 1;
    while (changed) {
        changed = 0;
        for (index := 0; index < count; index++) {
            part := visible[index];
            if (!used[index] && part != nil && touches(combined, part, doc.work)) {
                used[index] = 1;
                merged++;
                changed = 1;
                for (byteindex := 0; byteindex < GridBytes; byteindex++) {
                    if (byteindex % 512 == 0) {
                        checkwork(doc.work);
                    }
                    combined[byteindex] |= part[byteindex];
                }
            }
        }
    }
    replacement := array[MaxShapes] of ref Shape;
    retained := 0;
    total := 0;
    for (index := 0; index <= count; index++) {
        pieces: array of ref Shape = nil;
        if (index == count) {
            if (merged != 0) {
                exact = nil;
                for (old := 0; old < count; old++) {
                    if (used[old] && paintsame(combined, areas[old], doc.work)) {
                        pieces = array[] of {doc.shapes[old]};
                        break;
                    }
                }
            }
            if (pieces == nil) {
                pieces = paintislands(raw.color, combined, exact, doc.work);
            }
            if (pieces == nil) {
                return Limit;
            }
        } else if (!used[index]) {
            prior := doc.shapes[index];
            cut := paintsubtract(areas[index], combined, doc.work);
            if (cut == nil) {
                pieces = array[] of {prior};
            } else if (!paintempty(cut, doc.work)) {
                hole: array of Node = nil;
                if (merged == 0 && exact != nil && prior.fill && prior.width == 0) {
                    hole = painthole(prior.nodes, exact, footprint, areas[index], doc.work);
                }
                pieces = paintislands(prior.color, cut, hole, doc.work);
                if (pieces == nil) {
                    return Limit;
                }
                if (hole != nil && len prior.joins != 0) {
                    joins := array[len hole] of {* => 0};
                    joins[0:] = prior.joins;
                    pieces[0].joins = joins;
                }
            }
        }
        if (retained + len pieces > MaxShapes) {
            return Limit;
        }
        for (i := 0; i < len pieces; i++) {
            total += len pieces[i].nodes;
            if (total > MaxTotal) {
                return Limit;
            }
            replacement[retained++] = pieces[i];
        }
    }
    doc.shapes = replacement[0:retained];
    return Ok;
}

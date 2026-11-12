# Copyright (c) 2026 bhgv
# [GC:] EN: Keep real packet fairness probes under recoverable allocator pressure.
# RU: Проверять настоящие пакеты при обратимом отказе памяти вместо старого callback.
from concurrent_owner_fixture import expand


# [GC:] EN: Borrow root/work and return private C paths for fixture and RC.
# Keep mapped packets, physical frees, exact roots/counts, channel races and small
# budgets. A known callback-free record replaces the deliberately unknown type;
# refuse only its real leaf metadata allocation until independent work completes.
# No collector result, claim or managed owner is fabricated. Reject source drift.
# RU: Заимствовать root/work и вернуть личные пути C теста и RC. Сохранить пакеты,
# настоящие free, корни/ссылки, гонки каналов и малые бюджеты. Вместо неизвестного
# типа — обычная запись; отклонять лишь её native-метаданные до независимого free.
# Результаты GC, захваты и владельцы не подменяются. Изменение границ — отказ.
def prepare_packet_fixture(root, work):
    from rc_test_source import read_rc_source

    fixture = expand(root, (root / "test_packet_fairness.c").read_text())
    anchor = "#define MandatoryBlock (QueueFairness && CONTROL != 6 && CONTROL != 104)"
    if fixture.count(anchor) != 1:
        raise RuntimeError("packet unsupported scaffold anchor changed")
    fixture = fixture.replace(anchor, "#define MandatoryBlock 0")
    fixture = fixture.replace(
        "dtype(unknown_destructor, sizeof(void *), NULL, 0)",
        "dtype(freeheap, sizeof(void *), NULL, 0)",
    )
    marker = "static void publish_mixed(void **values)"
    if fixture.count(marker) != 1:
        raise RuntimeError("packet construction anchor changed")
    hook = r"""
static int packet_hold = 1; /* Fixture mutex guards the actual allocator refusal. */
static unsigned packet_refusals; /* Real failed native allocations, under sync. */

/* [GC:] EN: Borrow heap identity and allocate bytes normally except while its
 * selected packet owner must remain rooted. Refuse native metadata only.
 * RU: Заимствовать heap и обычно выделить bytes, кроме выбранного владельца
 * удерживаемого пакета. Отклоняется только native-память метаданных. */
void *dvm_test_packet_leaf_malloc(Heap *heap, size_t bytes)
{
    pthread_mutex_lock(&sync);
    int refuse = heap == targets[1] && packet_hold && CONTROL != 102;
    packet_refusals += refuse;
    pthread_mutex_unlock(&sync);
    return refuse ? NULL : malloc(bytes);
}

"""
    # The observer precedes publish_mixed, so its predicate needs an early declaration.
    fixture = fixture.replace(
        "static unsigned callbacks;", "static int packet_hold;\nstatic unsigned callbacks;", 1
    )
    fixture = fixture.replace(marker, hook + marker)
    fixture = fixture.replace("zero_passes >= 4 &&", "zero_passes >= 4 && packet_refusals >= 4 &&")
    fixture = fixture.replace(
        'require(released, "blocked suffix stays owned until retained cleanup");',
        r"""
                DVM_WorldState state;
                require(!packet_hold && !released && dvm_world_state(dvm_gc_runtime.world, &state) &&
                            !state.stopped && state.active >= 2,
                        "blocked packet owners must survive allocation refusal");""",
    )
    start = fixture.index(
        "    released = 1;\n    pthread_cond_broadcast(&changed);",
        fixture.index("void __wrap_Sys_werrstr"),
    )
    end = fixture.index("    require(dvm_refcount_load_i(&unknown_type->ref)", start)
    fixture = fixture[:start] + r"""
    require(packet_refusals >= 4, "real packet metadata allocations were refused");
    packet_hold = 0;
    pthread_mutex_unlock(&sync);
    require(dvm_gc_workers_request(dvm_gc_runtime.workers, DVM_GC_FAST) &&
                dvm_gc_workers_wait_idle(dvm_gc_runtime.workers), "shared packet cleanup completes");
    pthread_mutex_lock(&sync);
    require(!callbacks && freed[1] && freed[2], "supported blocked owners free once without exclusion");
    released = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&sync);
    dvm_runtime_gc_native_enter(native);
""" + fixture[end:]
    start = fixture.rindex("    fprintf(stderr,")
    fixture = (
        fixture[:start]
        + '    fprintf(stderr, "PASS concurrent packet owners and roots peers=2\\n");\n}\n'
    )
    fixture_path = work / "concurrent_packet.c"
    fixture_path.write_text(fixture)
    source = read_rc_source(root)
    begin = source.index("static int defer_leaf(")
    end = source.index("\n}\n", begin) + 3
    part = source[begin:end]
    anchor = "LeafRetirement *entry = malloc(sizeof(*entry));"
    if part.count(anchor) != 1:
        raise RuntimeError("packet real leaf allocator anchor changed")
    part = part.replace(
        anchor, "LeafRetirement *entry = dvm_test_packet_leaf_malloc(heap, sizeof(*entry));"
    )
    source = (
        source[:begin]
        + "extern void *dvm_test_packet_leaf_malloc(Heap *, size_t);\n\n"
        + part
        + source[end:]
    )
    return fixture_path, source

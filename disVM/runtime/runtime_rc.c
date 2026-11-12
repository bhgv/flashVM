/* Copyright (c) 2026 bhgv
 * EN: Compose private RC/GC modules in dependency order; one translation unit
 * preserves private state and callbacks. See gc/common/RC-SOURCE.md.
 * RU: Подключить закрытые модули RC/GC в порядке зависимостей; единая единица
 * трансляции сохраняет закрытые состояние и callbacks. См. gc/common/RC-SOURCE.md.
 */
#include "gc/common/rc_state.inc"
#include "gc/common/concurrent_policy.inc"
#include "gc/fast/leaf_publication.inc"
#include "gc/fast/graph_publication.inc"
#include "gc/cycles/candidate_publication.inc"
#include "gc/fast/owner_publication.inc"
#include "gc/fast/frame_publication.inc"
#include "gc/fast/array_children.inc"
#include "gc/cycles/packet_rescue.inc"
#include "gc/fast/packet_children.inc"
#include "gc/fast/module_data.inc"
#include "gc/fast/object_children.inc"
#include "gc/fast/graph_release.inc"
#include "gc/cycles/candidate_transfer.inc"
#include "gc/duke/pin_release.inc"
#include "gc/duke/rooted_candidates.inc"
#include "gc/cycles/candidate_scan.inc"
#include "gc/cycles/rc_passes.inc"
#include "gc/duke/rc_passes.inc"
#include "gc/cycles/owner_rescue.inc"
#include "gc/fast/owner_drain.inc"
#include "gc/fast/work_budget.inc"
#include "gc/fast/leaf_release.inc"
#include "gc/fast/shared_release.inc"
#include "gc/fast/retained_release.inc"
#include "gc/common/rc_lifecycle.inc"

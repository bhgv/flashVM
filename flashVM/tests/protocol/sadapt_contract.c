/* Native adaptation boundaries, overflow-safe counters, pending negotiation and feedback codec. */
#include "flashvm/sadapt.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "adapt:%d %s\n", __LINE__, #x);                                                            \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


int main(void)
{
    FVM_SControlLimits profiles[3] = {{19, 480, 512, 400}, {19, 992, 1024, 768}, {19, 4064, 4096, 4000}};
    FVM_SAdaptConfig config;
    FVM_SAdapt state, before;
    FVM_SAdaptDecision decision, saved;
    FVM_SAdaptFeedback f = {7, 8, 1, 100, 80, 10, 5, 1, 20}, decoded;
    unsigned char bytes[128];
    size_t used, consumed;
    CHECK(fvm_sadapt_config_init(&config) == FVM_S_OK);
    config.profiles = profiles;
    config.count = 3;
    config.initial = 1;
    config.stream = 7;
    config.epoch = 8;
    CHECK(fvm_sadapt_init(&state, &config) == FVM_S_OK);
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_KEEP);
    ++f.window;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_SHRINK &&
          state.current == 1 && state.pending == 0);
    before = state;
    saved = decision;
    ++f.window;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_CONFLICT && !memcmp(&state, &before, sizeof(state)) &&
          !memcmp(&decision, &saved, sizeof(decision)));
    CHECK(fvm_sadapt_commit(&state, profiles + 2) == FVM_S_CONFLICT && state.current == 1);
    CHECK(fvm_sadapt_commit(&state, profiles) == FVM_S_OK && state.current == 0);
    f.expected = f.on_time = 1;
    f.late = f.damaged = 0;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_INSUFFICIENT &&
          state.cooldown == 2);
    f.expected = f.on_time = 100;
    ++f.window;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.reasons == FVM_SADAPT_COOLDOWN &&
          state.cooldown == 1);
    ++f.window;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && !state.cooldown);
    for (consumed = 0; consumed < 4; ++consumed)
    {
        ++f.window;
        CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK);
    }
    CHECK(decision.action == FVM_SADAPT_GROW && decision.candidate == 1 && state.current == 0);
    CHECK(fvm_sadapt_reject(&state) == FVM_S_OK && state.current == 0 && state.cooldown == 2);
    CHECK(fvm_sadapt_feedback_write(&f, bytes, sizeof(bytes), &used) == FVM_S_OK && used < sizeof(bytes));
    CHECK(fvm_sadapt_feedback_read(bytes, used, &decoded, &consumed) == FVM_S_OK && consumed == used &&
          !memcmp(&decoded, &f, sizeof(f)));
    before = state;
    saved = decision;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_CONFLICT && !memcmp(&state, &before, sizeof(state)));
    ++f.window;
    f.on_time = FVM_SRECORD_U32;
    f.expected = 100;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_INVALID && !memcmp(&decision, &saved, sizeof(decision)));
    /* The exact percentage boundary must work on 32-bit unsigned long without a wider multiply. */
    config.cooldown_windows = 0;
    config.bad_windows = config.good_windows = 1;
    config.min_expected = 1;
    CHECK(fvm_sadapt_init(&state, &config) == FVM_S_OK);
    f.window = 1;
    f.expected = FVM_SRECORD_U32;
    f.on_time = FVM_SRECORD_U32 - 429496729UL;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_KEEP);
    ++f.window;
    --f.on_time;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_SHRINK);
    CHECK(fvm_sadapt_reject(&state) == FVM_S_OK);
    ++f.window;
    f.expected = f.on_time = 100;
    f.rtt_known = 0;
    f.rtt_ms = 0;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_KEEP);
    ++f.window;
    f.rtt_known = 1;
    f.rtt_ms = 250;
    CHECK(fvm_sadapt_observe(&state, &f, &decision) == FVM_S_OK && decision.action == FVM_SADAPT_SHRINK &&
          decision.reasons == FVM_SADAPT_LATENCY);
    puts("PASS bounded adaptation, pending commit/reject, cooldown, counts and portable feedback");
    return 0;
}

# Mixed event scheduling through stream gates


v0.987: Refactor native stream work beneath one callback barrier while preserving base retry, frame gates and EOF-tail behavior.

v0.988: Add paired pump_mix_until: choose authorized base or event-only work while retaining source position and coherent clocks.

v0.989: Verify tail retry before newly authorized base, held base retention, callback recursion and backend exception containment.

v0.990: Route mixed worker steps through paired gate scheduling, preserve retry priority and activate source successors before synthetic events.

v0.991: Separate completed-frame publication from base authorization; root Stop holds stream phase while events continue and host pause still suspends the queue.

v0.992: Exercise seeded 2^53 frame boundaries, backend/clock rejection and zero-allocation gate/retry/clear paths after native preparation.

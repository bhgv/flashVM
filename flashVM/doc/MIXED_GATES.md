# Mixed event scheduling through stream gates


v0.987: Refactor native stream work beneath one callback barrier while preserving base retry, frame gates and EOF-tail behavior.

v0.988: Add paired pump_mix_until: choose authorized base or event-only work while retaining source position and coherent clocks.

v0.989: Verify tail retry before newly authorized base, held base retention, callback recursion and backend exception containment.

v0.990: Route mixed worker steps through paired gate scheduling, preserve retry priority and activate source successors before synthetic events.

v0.991: Separate completed-frame publication from base authorization; root Stop holds stream phase while events continue and host pause still suspends the queue.

v0.992: Exercise seeded 2^53 frame boundaries, backend/clock rejection and zero-allocation gate/retry/clear paths after native preparation.

v0.993: Run 256 native source-teardown epochs with rejected tail retries, alternating gates, exact PCM witnesses and coherent dequeue/ID state.

v0.994: Keep held BASE retries intact, drain accepted lead, preserve failed clear and admit fresh stopped-root events only after staged cancellation.

v0.995: Add a seventh gated export mode with exact 17-frame PCM, held converter witness and deliberately rejected tail-to-base gate transitions.

v0.996: Exercise real SDL host pause, authored Stop, GC/native-JIT execution, event-only drain and subsequent base resume without converter rewind.

v0.997: Check exact event-only WAV for FWS/CWS in both VM modes under future stream gates and root Stop, plus mute without an audio device.

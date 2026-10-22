# Queued identities for stream/event composition

v0.951 prepares guarded queue observation for tracked admissions: the internal observe_locked helper operates only inside a public reentry scope. Ordinary adapted.queued retains its existing validation, output preservation and exception handling. Existing clock, tail and callback-control regressions are rerun. No queued identity API is enabled by this refactor alone. Native owners stay externally serialized and hold no managed/GC reference. Full desktop integration, devices/platforms and all roadmap stages remain open.

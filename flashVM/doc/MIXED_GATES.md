# Mixed event scheduling through stream gates


v0.987: Refactor native stream work beneath one callback barrier while preserving base retry, frame gates and EOF-tail behavior.

v0.988: Add paired pump_mix_until: choose authorized base or event-only work while retaining source position and coherent clocks.

v0.989: Verify tail retry before newly authorized base, held base retention, callback recursion and backend exception containment.

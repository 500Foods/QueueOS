# Implementation plans

This directory holds implementation plans.

A plan describes a slice of work that is ready to build: what is in scope, what is explicitly deferred, the order of the work, and how the result will be validated.

`brainstorming/` remains the architectural exploration. A plan may adopt a direction from those notes. Writing a plan does not by itself turn an open question into an accepted decision.

Plans are added as separate documents when a slice of work is ready to implement.

- [001 — Harness and stubs](001-harness-and-stubs.md). The first plan. First batch closed: the QEMU harness, the two disk files, and the trivial Boot and Base programs. The record keeps the original ask, with corrections beside the sentences the boots made false.
- [002 — Second batch](002-second-batch.md). Closed on 2026-11-24. Six cases passed in `results/000-007`. The `pc` hand boot is `results/000-006`. The following first batch is `results/000-008`. It is also the diary of what plan 001 got wrong.
- [003 — Slots and the publisher](003-slots-and-publisher.md). Ready to build on 2026-11-25. This is the active slice. Nothing in it is built.

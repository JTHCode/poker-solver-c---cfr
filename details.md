# Collaboration Notes

- Use the **Dev Notes** subsections in `plan.md` to record findings, quirks, and guidance while working through each checklist step. Keep entries concise and dated.
- When making substantial edits to existing code (beyond trivial fixes), update `changelog.md` with a brief, dated summary of what changed and why.

## Phase C1 Decisions for `upgrade-plan.md`:
- stack size model: bb
- streets modeled: flop/turn/river. Prefer `preflop-ranges/` for preflop modeling rather than raw logic calculations
- bet sizes allowed per street: 33%, 66%, pot, all-in
- raise sizes allowed: 2.5x, 3.25x, 4x, all-in
- max raises per street: 3 (knows as 4-bet in poker terms)
- “reopen action” handling for short all-ins: simplified rules

## Phase D2 Info-Set Key (Current)

Current info-set key uses only **public** information and is intended for this project’s current toy abstraction:
- street
- public board cards known so far (`board_count` + cards)
- public betting state (pot, to_call, per-street committed, last_bet_size, raises_this_street, reopen_allowed, consecutive_checks)
- player-to-act / owner

Notes:
- Action-history is not stored explicitly yet; the current key is a function of the public betting totals/state instead.
- Private hole cards and bucketing are not represented yet (future work in later phases).

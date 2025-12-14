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
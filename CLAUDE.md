# CLAUDE.md

Guidance for Claude Code (and any human contributor) working in this repository.

## Project

A C++20 command-line step sequencer with a built-in synth engine, built as a portfolio
project. See [DECISIONS.md](./DECISIONS.md) for the reasoning behind key technical choices —
keep that file updated as we go, not just this one.

## Current milestone (v1 — small and contained)

- REPL with tracker-style pattern input, e.g. `kick x..x..x..x..`
- Small fixed voice set: kick, snare, hat, one synth voice; 16-step pattern
- Built-in synth (oscillator + envelope + mixer) played live via miniaudio
- MIDI export to `.mid`
- Save/load pattern as JSON (nlohmann/json)
- Catch2 unit tests, CMake build

## Progress so far

- [x] Repo scaffolding: license, gitignore, README, CMake build, Catch2 test harness
- [x] `Step` type (`include/stepseq/step.hpp`) — trigger-only for now, no note field
- [x] `Track` type (a named voice holding a fixed-length sequence of `Step`s)
- [x] `Pattern` type (multiple `Track`s + tempo)
- [ ] REPL for tracker-style pattern input — next up
- [ ] Synth engine (oscillator + envelope + mixer) + miniaudio playback
- [ ] MIDI export
- [ ] Save/load pattern as JSON

## v1.1 (later — do not pull forward into v1)

- Live terminal grid UI (FTXUI)
- MIDI import

## Workflow rules

- **Granular commits/PRs.** Keep changes small and frequent, each one a coherent, working
  step. Mimic incremental human development so the history is easy to bisect or roll back,
  and reads as a series of problems solved along the way rather than one large drop.
- **Keep commit messages human-sounding.** Write commit messages the way an engineer would
  describe the problem and the fix.
- **Update DECISIONS.md alongside any commit that makes a notable technical choice**
  (data structure, library, algorithm, file format, etc.) — one succinct entry: what was
  chosen and why, plus what was ruled out if relevant. Skip decisions that are obvious or
  derivable from the code.
- **Peripheral work moves fast.** Repo housekeeping, git/GitHub administration, CI, docs,
  and other non-C++ scaffolding can proceed with just a quick confirmation — no deep review
  loop needed.
- **C++ source code goes through a stricter loop.** For any change to the actual sequencer
  code: (1) write a small, single-concept change; (2) have a separate agent perform an
  independent code review before presenting it; (3) walk through the change together and
  question the user on every non-trivial line and decision — not a quick 1–3 question check,
  but enough that they could explain the code as if they'd written it themselves; (4) do not
  start the next piece of code until that understanding is demonstrated — if they can't
  explain it, re-explain and re-ask rather than moving on.
- **Walkthroughs double as interview prep.** During step (3) above, extend questions into
  adjacent C++ concepts the change touches (initialization order, namespaces, linkage,
  value categories, etc.), not just the literal diff — this project exists partly to prepare
  for C++ interviews, so testing understanding more broadly is valuable, not scope creep.

## Build & test

TBD — filled in once the repo is scaffolded.

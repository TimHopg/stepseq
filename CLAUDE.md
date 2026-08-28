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

## v1.1 (later — do not pull forward into v1)

- Live terminal grid UI (FTXUI)
- MIDI import

## Workflow rules

- **Granular commits/PRs.** Keep changes small and frequent, each one a coherent, working
  step. Mimic incremental human development so the history is easy to bisect or roll back,
  and reads as a series of problems solved along the way rather than one large drop.
- **Don't mask AI involvement, but keep messages human-sounding.** Write commit messages
  the way an engineer would describe the problem and the fix — no need to announce
  "written by Claude" in the body. The `Co-Authored-By: Claude Sonnet 5` trailer on every
  commit is the authorship record; never omit it.
- **Update DECISIONS.md alongside any commit that makes a notable technical choice**
  (data structure, library, algorithm, file format, etc.) — one succinct entry: what was
  chosen and why, plus what was ruled out if relevant. Skip decisions that are obvious or
  derivable from the code.
- **Comprehension check after each commit/PR.** Once a commit (or PR) is complete, ask 1–3
  questions about the concepts or code just introduced. Do not start the next piece of work
  until the user answers correctly — if they can't, explain and re-ask rather than moving on.

## Build & test

TBD — filled in once the repo is scaffolded.

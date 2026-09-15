# WIP handover — the `stepseq_lib` static library split

Branch: `wip/library-split`. Everything here builds and passes (71/71), but **the C++ has not
been walked through yet**, so per CLAUDE.md it is not ready to merge to `main`. Pick up by
reading this file, then asking for the walkthrough.

---

## 1. Background you asked for: binaries, object files, archives, linking

I used the phrase "both binaries link one archive" and it deserved unpacking. Building up from
the bottom.

### A translation unit

One `.cpp` file after the preprocessor has pasted in every `#include`. It is the unit the
compiler actually reads. Headers are never compiled — their *text* is pasted into each `.cpp`
that includes them, and each of those copies gets compiled separately. (`src/main.cpp` is 11
lines and becomes ~45,000 after this pasting, nearly all standard library.)

### An object file (`.o`)

What you get from compiling one translation unit. It contains machine code, plus a **symbol
table** — a list of named things. Two kinds matter:

- things this file **defines** (here is the code for `parseSteps`)
- things this file **needs but does not have** (I call `parseSteps`; somebody else must supply it)

An object file on its own is not runnable. It has holes where the calls to other files go.

### Linking

The linker takes a pile of object files, matches every "I need X" against somebody's "I define
X", fills in the holes with real addresses, and writes out one runnable executable. If nothing
defines X → `undefined reference to X`. If *two* things define X → `multiple definition of X`.
That second rule is the **One Definition Rule**, and it is the whole reason every function in
this repo was marked `inline`: `inline` means "this may appear in many translation units, they
are all identical, pick one and bin the rest." It was never about speed.

### An archive (`.a`) — the "static library"

A bundle of object files in one file, with an index. `libstepseq_lib.a` is exactly that. When
the linker is handed an archive it does not blindly include everything — it pulls out only the
members that satisfy a symbol somebody actually needs. "Static" means the code is copied *into*
each executable at link time, so the finished binary has no runtime dependency on the archive.
(A *shared* library, `.so`/`.dll`, is the alternative: resolved when the program starts, not
when it is built. We are not using one.)

### Why "both binaries"

The build produces **two separate executables**:

| Binary | Built from | Has `main()` from |
|---|---|---|
| `stepseq` | `src/main.cpp` | our `src/main.cpp` |
| `stepseq_tests` | the 7 files in `tests/` | Catch2 |

They share no compiled code by default — CMake would compile the same sources twice, once for
each. And crucially `stepseq_tests` **cannot** include `src/main.cpp`, because Catch2 supplies
its own `main()` and you cannot have two.

So "both binaries link one archive" means: compile the shared sequencer code **once** into
`libstepseq_lib.a`, then let each executable pull what it needs out of it. One compile, two
consumers.

### Reading `nm` output

`nm -C somefile.o` lists the symbol table. The letter before each name is the interesting part:

- **`T`** — defined here, in the text (code) section. One real definition.
- **`U`** — undefined. "I need this, resolve it at link time."
- **`W`** — weak. "Defined here, but others may define it too; dedupe us." This is what
  `inline` produces.

After the split:

```
libstepseq_lib.a          →  T stepseq::parseSteps(...)     one definition, in the archive
main.cpp.o                →  U stepseq::parseSteps(...)     just a reference
steps_parser_test.cpp.o   →  U stepseq::parseSteps(...)     just a reference
```

Before the split it was `W` in every object file that included the header — a full duplicate
copy in each, with the linker throwing away all but one. That is what changed.

---

## 2. Why this was done now rather than later

miniaudio (the audio library, next milestone item) ships as a single header whose function
bodies are fenced behind `#define MINIAUDIO_IMPLEMENTATION`. Those bodies are ordinary C
functions — **not** `inline` — so they must be compiled in **exactly one translation unit per
executable**. Zero → undefined references. Two → multiple definition.

Before this change, the only non-test `.cpp` was `src/main.cpp`, which `stepseq_tests` cannot
link. So there was nowhere to put the implementation that both binaries could reach. The static
library is the fix, and it had to land *before* any playback code so the build change and the
audio logic stay in separate commits.

Once we add audio, it is one line: drop `src/miniaudio_impl.cpp` (containing nothing but the
`#define` and the `#include`) into the `add_library` list.

---

## 3. What actually changed on this branch

- **`CMakeLists.txt`** — new `stepseq_lib` STATIC target. Both executables now
  `target_link_libraries(... stepseq_lib)`. The include directory moved onto the library as
  `PUBLIC` (consumers inherit it, so the duplicated `target_include_directories` on each
  executable is gone); warning flags stay `PRIVATE` (how we compile ourselves, not a rule we
  impose on anyone linking us).
- **`include/stepseq/steps_parser.hpp`** — now a *declaration only*. Lost the function body,
  lost `inline`, lost `<stdexcept>` and `<string>` (the declaration does not need them).
- **`src/steps_parser.cpp`** — NEW. Holds the definition, non-`inline`.
- **`tests/steps_parser_test.cpp`** — gained `#include <stdexcept>`. It was using
  `std::invalid_argument` without including it, working only because the old header happened to
  include it and pass it along. Slimming the header exposed that.
- **`DECISIONS.md`** — one entry covering all of the above.

### Deliberately NOT split

- `Step`, `Track` — plain structs, no function bodies to move.
- `Oscillator` — **stays header-only on purpose.** `nextSample` runs once per audio sample on
  the callback's deadline; a translation-unit boundary would cost cross-TU inlining on the
  hottest path in the project, for a four-line function that depends on nothing.
- `Pattern`, `repl.hpp` — still header-defined. Moving them is optional polish, needs no
  deadline, and bundling it would have made this diff much harder to check. **Open question,
  see below.**

---

## 4. Still to go over (the actual TODO)

1. **Walk through the split** — the linkage question especially: why `repl.hpp`'s `inline`
   functions may legally call a non-`inline` function that lives in a separate archive.
2. **Decide the stopping point.** Is "`parseSteps` split, `Pattern`/`repl` not" a coherent
   resting state, or does it read as half-finished to someone finding the repo? Argument for
   stopping: the library exists, which is all miniaudio needs. Argument for continuing:
   consistency, and `repl.hpp` is where most of the code is.
3. **Two one-liners already on `main` that were committed as trivial and never walked through** —
   worth two minutes each:
   - `8ec7089` `static_assert(kTracksPerPattern == 4)` in `makeDefaultPattern`.
   - `5cc80c4` `std::to_string(kStepsPerTrack)` in the `parseSteps` error message — note this
     allocates on the throw path, which is worth a sentence.

## 5. After that

The synth engine + miniaudio playback. First real design question there, before any code:
`play` is the first command that needs state outliving a single dispatch (an audio device), so
does that device get constructed in `main` and passed into `runRepl` alongside the `Pattern`,
or owned by the REPL itself?

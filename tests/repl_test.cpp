#include <catch2/catch_test_macros.hpp>

#include <array>
#include <sstream>
#include <string>
#include <utility>

#include <stepseq/pattern.hpp>
#include <stepseq/repl.hpp>
#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace {

// Expected output is kBanner + (kPrompt + command output)... + kEofTail, or
// nothing after the prompt for quit/exit. One test below pins the banner text.
const std::string kBanner{stepseq::kBanner};
const std::string kPrompt = "> ";

// The prompt before the read that hits end-of-input, plus its closing newline.
const std::string kEofTail = "> \n";

// What `print` renders for a freshly-made test pattern with no active steps.
const std::string kEmptyPatternOutput =
    "bpm: 120\n"
    "kick: ................\n"
    "snare: ................\n"
    "hat: ................\n"
    "synth: ................\n";

// Deliberately not makeDefaultPattern(): these tests pin exact output, so
// changing the v1 voice set should not break command-loop tests.
stepseq::Pattern makeTestPattern() {
    std::array<stepseq::Track, stepseq::kTracksPerPattern> tracks{};
    tracks[0].name = "kick";
    tracks[1].name = "snare";
    tracks[2].name = "hat";
    tracks[3].name = "synth";
    return stepseq::Pattern(120.0, std::move(tracks));
}

std::string runReplOn(const std::string& input, stepseq::Pattern& pattern) {
    std::istringstream in(input);
    std::ostringstream out;
    stepseq::runRepl(in, out, pattern);
    return out.str();
}

} // namespace

TEST_CASE("makeDefaultPattern builds the v1 voice set at the default tempo") {
    const stepseq::Pattern pattern = stepseq::makeDefaultPattern();

    // Spelled out rather than compared against kDefaultBpm, so changing the
    // default has to be a deliberate decision.
    REQUIRE(pattern.bpm() == 120.0);
    REQUIRE(pattern.tracks[0].name == "kick");
    REQUIRE(pattern.tracks[1].name == "snare");
    REQUIRE(pattern.tracks[2].name == "hat");
    REQUIRE(pattern.tracks[3].name == "synth");

    for (const stepseq::Track& track : pattern.tracks) {
        for (const stepseq::Step& step : track.steps) {
            REQUIRE_FALSE(step.active);
        }
    }
}

TEST_CASE("runRepl opens with the banner, before the first prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\n", pattern) ==
            "stepseq - 'print' shows the pattern, 'quit' exits.\n"
            "> ");
}

TEST_CASE("runRepl returns on quit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl returns on exit without a further prompt") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("exit\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl stops reading once it sees quit") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("quit\nbogus\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("runRepl closes the dangling prompt line at end of input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("", pattern) == kBanner + kEofTail);
}

TEST_CASE("runRepl handles a final line with no trailing newline") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl strips the trailing carriage return from CRLF input") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\r\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("bogus\r\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: bogus\n" + kEofTail);
}

TEST_CASE("runRepl skips blank and whitespace-only lines") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("\n   \n\t\n", pattern) ==
            kBanner + kPrompt + kPrompt + kPrompt + kEofTail);
}

TEST_CASE("runRepl reports an unknown command and keeps going") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("bogus\nalsobogus\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: bogus\n" + kPrompt +
                "error: unknown command: alsobogus\n" + kEofTail);
}

TEST_CASE("runRepl keeps going after a successful command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\nbogus\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kPrompt +
                "error: unknown command: bogus\n" + kEofTail);
}

TEST_CASE("runRepl commands are case-sensitive") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("PRINT\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: PRINT\n" + kEofTail);
    REQUIRE(runReplOn("Quit\n", pattern) ==
            kBanner + kPrompt + "error: unknown command: Quit\n" + kEofTail);
}

TEST_CASE("runRepl ignores surrounding whitespace around a command") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("   print   \n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("runRepl ignores extra tokens after a command that takes no arguments") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print junk\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
    REQUIRE(runReplOn("quit junk\n", pattern) == kBanner + kPrompt);
}

TEST_CASE("print renders the bpm and every track") {
    stepseq::Pattern pattern = makeTestPattern();

    REQUIRE(runReplOn("print\n", pattern) ==
            kBanner + kPrompt + kEmptyPatternOutput + kEofTail);
}

TEST_CASE("print renders active steps as 'x'") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.tracks[0].steps[0].active = true;
    pattern.tracks[0].steps[4].active = true;

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("kick: x...x...........\n") != std::string::npos);
}

TEST_CASE("print reflects a changed bpm") {
    stepseq::Pattern pattern = makeTestPattern();
    pattern.setBpm(140.0);

    const std::string output = runReplOn("print\n", pattern);

    REQUIRE(output.find("bpm: 140\n") != std::string::npos);
}

TEST_CASE("printPattern can be called directly on a const Pattern") {
    const stepseq::Pattern pattern = makeTestPattern();
    std::ostringstream out;

    stepseq::printPattern(out, pattern);

    REQUIRE(out.str() == kEmptyPatternOutput);
}

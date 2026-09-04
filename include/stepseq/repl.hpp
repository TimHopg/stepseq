#pragma once

#include <array>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <stepseq/pattern.hpp>
#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace stepseq {

inline constexpr std::string_view kBanner =
    "stepseq - 'print' shows the pattern, 'quit' exits.\n";
inline constexpr std::string_view kPrompt = "> ";
inline constexpr double kDefaultBpm = 120.0;

// v1 voice set, all steps inactive. Pattern does not know which voices, so it
// lives here, not in pattern.hpp.
inline Pattern makeDefaultPattern() {
    std::array<Track, kTracksPerPattern> tracks{};
    tracks[0].name = "kick";
    tracks[1].name = "snare";
    tracks[2].name = "hat";
    tracks[3].name = "synth";
    return Pattern(kDefaultBpm, std::move(tracks));
}

inline void printPattern(std::ostream& out, const Pattern& pattern) {
    out << "bpm: " << pattern.bpm() << '\n';
    for (const Track& track : pattern.tracks) {
        out << track.name << ": ";
        for (const Step& step : track.steps) {
            out << (step.active ? 'x' : '.');
        }
        out << '\n';
    }
}

inline void runRepl(std::istream& in, std::ostream& out, Pattern& pattern) {
    out << kBanner;

    std::string line;
    while (true) {
        out << kPrompt;
        if (!std::getline(in, line)) {
            // End of input (Ctrl-D): close the dangling prompt line.
            out << '\n';
            return;
        }

        // getline splits on '\n' only, so CRLF input leaves a trailing '\r'.
        // Strip it here rather than relying on each command to tolerate it.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::istringstream words(line);
        std::string command;
        if (!(words >> command)) {
            continue;
        }

        if (command == "quit" || command == "exit") {
            return;
        }

        if (command == "print") {
            printPattern(out, pattern);
            continue;
        }

        out << "error: unknown command: " << command << '\n';
    }
}

} // namespace stepseq

#pragma once

#include <array>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <stepseq/pattern.hpp>
#include <stepseq/step.hpp>
#include <stepseq/steps_parser.hpp>
#include <stepseq/track.hpp>

namespace stepseq {

inline constexpr std::string_view kBanner =
    "stepseq - 'kick x..x..x..x..x..x' sets steps, 'print' shows the pattern, "
    "'quit' exits.\n";
inline constexpr std::string_view kPrompt = "> ";
inline constexpr double kDefaultBpm = 120.0;
inline constexpr double kMinBpm = 20.0;
inline constexpr double kMaxBpm = 300.0;

// v1: lives here since Pattern does not know or care about which tracks it has
inline Pattern makeDefaultPattern() {
    std::array<Track, kTracksPerPattern> tracks{};
    tracks[0].name = "kick";
    tracks[1].name = "snare";
    tracks[2].name = "hat";
    tracks[3].name = "synth";
    return Pattern(kDefaultBpm, std::move(tracks));
}

inline constexpr std::size_t kLabelWidth = 8;

static_assert(kLabelWidth >= 2, "kLabelWidth must leave room for ':' and a space");
inline constexpr std::size_t kMaxLabelNameWidth = kLabelWidth - 2;

namespace detail {

inline void printLabel(std::ostream& out, std::string_view name) {
    const std::string_view shown = name.substr(0, kMaxLabelNameWidth);
    out << shown << ':' << std::string(kLabelWidth - shown.size() - 1, ' ');
}

inline void printBpm(std::ostream& out, const Pattern& pattern) {
    printLabel(out, "bpm");
    out << pattern.bpm() << '\n';
}

} // namespace detail

inline void printPattern(std::ostream& out, const Pattern& pattern) {
    detail::printBpm(out, pattern);
    for (const Track& track : pattern.tracks) {
        detail::printLabel(out, track.name);
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
        if (command == "bpm") {
            std::string value_text;
            if (!(words >> value_text)) {
                detail::printBpm(out, pattern);
                continue;
            }
            std::istringstream value_stream(value_text);
            double value = 0.0;
            std::string leftover;
            if (!(value_stream >> value) || !value_stream.eof() || (words >> leftover)) {
                out << "error: 'bpm' takes one number, e.g. 'bpm 140'\n";
                continue;
            }
            if (!(value >= kMinBpm && value <= kMaxBpm)) {
                out << "error: bpm must be between " << kMinBpm << " and " << kMaxBpm
                    << '\n';
                continue;
            }
            pattern.setBpm(value);
            continue;
        }
        // Checked after the built-ins, so a track could never shadow a command.
        if (Track* track = pattern.findTrack(command)) {
            // 'xxxx xxxx xxxx xxxx' is one pattern; joining the tokens drops the spaces.
            std::string steps_text;
            for (std::string group; words >> group;) {
                steps_text += group;
            }
            if (steps_text.empty()) {
                out << "error: '" << command << "' needs a step pattern, e.g. '"
                    << command << " x..x..x..x..x..x'\n";
                continue;
            }
            try {
                track->steps = parseSteps(steps_text);
            } catch (const std::invalid_argument&) {
                out << "error: step pattern must be " << kStepsPerTrack
                    << " steps of 'x' or '.' (spaces between groups are ignored)\n";
            }
            continue;
        }

        out << "error: unknown command: " << command << '\n';
    }
}

} // namespace stepseq

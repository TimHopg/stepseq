#pragma once

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include <stepseq/pattern.hpp>
#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace stepseq {

inline constexpr std::string_view kPrompt = "> ";

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
    std::string line;
    while (true) {
        out << kPrompt;
        if (!std::getline(in, line)) {
            // End of input (Ctrl-D). The cursor is sitting just after the
            // prompt, so close the line before handing the terminal back.
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

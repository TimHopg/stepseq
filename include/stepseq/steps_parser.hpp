#pragma once

#include <array>
#include <string_view>

#include <stepseq/step.hpp>
#include <stepseq/track.hpp>

namespace stepseq {

// Throws std::invalid_argument unless the pattern is exactly kStepsPerTrack
// characters of 'x' or '.'.
std::array<Step, kStepsPerTrack> parseSteps(std::string_view pattern);

} // namespace stepseq

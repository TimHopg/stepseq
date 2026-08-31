#pragma once

#include <array>
#include <string>

#include <stepseq/step.hpp>

namespace stepseq {

inline constexpr std::size_t kStepsPerTrack = 16;

struct Track {
    std::string name;
    std::array<Step, kStepsPerTrack> steps{};
};

} // namespace stepseq

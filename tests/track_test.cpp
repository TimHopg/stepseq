#include <catch2/catch_test_macros.hpp>

#include <stepseq/track.hpp>

TEST_CASE("kStepsPerTrack is 16") {
    REQUIRE(stepseq::kStepsPerTrack == 16);
}

TEST_CASE("a default-constructed Track has kStepsPerTrack inactive steps and no name") {
    const stepseq::Track track;
    REQUIRE(track.name.empty());
    REQUIRE(track.steps.size() == stepseq::kStepsPerTrack);
    for (const auto& step : track.steps) {
        REQUIRE_FALSE(step.active);
    }
}

TEST_CASE("a Track can be named") {
    const stepseq::Track track{.name = "kick"};
    REQUIRE(track.name == "kick");
}

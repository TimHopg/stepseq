#include <catch2/catch_test_macros.hpp>

#include <stepseq/steps_parser.hpp>

TEST_CASE("parseSteps turns 'x' into an active step and '.' into an inactive one") {
    const auto steps = stepseq::parseSteps("x..x..x..x..x..x");

    REQUIRE(steps[0].active);
    REQUIRE_FALSE(steps[1].active);
    REQUIRE_FALSE(steps[2].active);
    REQUIRE(steps[3].active);
    REQUIRE(steps[15].active);
}

TEST_CASE("parseSteps rejects a pattern that isn't kStepsPerTrack characters") {
    REQUIRE_THROWS_AS(stepseq::parseSteps("x."), std::invalid_argument);
    REQUIRE_THROWS_AS(stepseq::parseSteps("x...............x"), std::invalid_argument);
}

TEST_CASE("parseSteps rejects a character that isn't 'x' or '.'") {
    REQUIRE_THROWS_AS(stepseq::parseSteps("x..x..x..x..x..?"), std::invalid_argument);
}

TEST_CASE("parseSteps rejects uppercase 'X', since it is case-sensitive") {
    REQUIRE_THROWS_AS(stepseq::parseSteps("X..x..x..x..x..x"), std::invalid_argument);
}

TEST_CASE("parseSteps accepts a pattern that is all 'x'") {
    const auto steps = stepseq::parseSteps("xxxxxxxxxxxxxxxx");

    for (const auto& step : steps) {
        REQUIRE(step.active);
    }
}

TEST_CASE("parseSteps accepts a pattern that is all '.'") {
    const auto steps = stepseq::parseSteps("................");

    for (const auto& step : steps) {
        REQUIRE_FALSE(step.active);
    }
}

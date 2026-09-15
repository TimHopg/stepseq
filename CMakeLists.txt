cmake_minimum_required(VERSION 3.20)
project(StepSequencer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Deliberately not -Wshadow: constructors here take a parameter named after the
# member it initializes, which is idiomatic and would warn on every one.
if(MSVC)
    set(STEPSEQ_WARNINGS /W4)
else()
    set(STEPSEQ_WARNINGS -Wall -Wextra -Wpedantic)
endif()

# The sequencer's compiled sources, linked by both the app and the tests.
add_library(stepseq_lib STATIC
    src/steps_parser.cpp
)
target_include_directories(stepseq_lib PUBLIC include)
target_compile_options(stepseq_lib PRIVATE ${STEPSEQ_WARNINGS})

add_executable(stepseq src/main.cpp)
target_link_libraries(stepseq PRIVATE stepseq_lib)
target_compile_options(stepseq PRIVATE ${STEPSEQ_WARNINGS})

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.9.1
)
FetchContent_MakeAvailable(Catch2)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(stepseq_tests
    tests/smoke_test.cpp
    tests/step_test.cpp
    tests/track_test.cpp
    tests/pattern_test.cpp
    tests/steps_parser_test.cpp
    tests/repl_test.cpp
    tests/oscillator_test.cpp
)
target_link_libraries(stepseq_tests PRIVATE stepseq_lib Catch2::Catch2WithMain)
target_compile_options(stepseq_tests PRIVATE ${STEPSEQ_WARNINGS})

catch_discover_tests(stepseq_tests)

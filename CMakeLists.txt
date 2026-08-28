cmake_minimum_required(VERSION 3.20)
project(StepSequencer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(stepseq src/main.cpp)

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

add_executable(stepseq_tests tests/smoke_test.cpp)
target_link_libraries(stepseq_tests PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(stepseq_tests)

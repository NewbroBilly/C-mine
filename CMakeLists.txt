cmake_minimum_required(VERSION 3.20)
project(Minecraft1122CppEdition VERSION 0.1.0 LANGUAGES CXX)

option(MC_BUILD_TESTS "Build core tests" ON)

add_library(mc_core
    src/core/chunk.cpp
    src/core/fluid_simulator.cpp
    src/core/player_physics.cpp
    src/core/world.cpp
)
target_include_directories(mc_core PUBLIC include)
target_compile_features(mc_core PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(mc_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(mc_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(minecraft_1122 src/main.cpp)
target_link_libraries(minecraft_1122 PRIVATE mc_core)

if(MC_BUILD_TESTS)
    enable_testing()
    add_executable(mc_core_tests tests/core_tests.cpp)
    target_link_libraries(mc_core_tests PRIVATE mc_core)
    add_test(NAME mc_core_tests COMMAND mc_core_tests)
endif()

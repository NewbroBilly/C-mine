cmake_minimum_required(VERSION 3.20)
project(Minecraft1122CppEdition VERSION 0.1.0 LANGUAGES CXX)
if(MSVC)
    add_compile_options(/utf-8)
endif()

option(MC_BUILD_TESTS "Build core tests" ON)
if(MC_BUILD_TESTS)
    enable_testing()
endif()
option(MC_BUILD_CLIENT "Build the SDL/OpenGL desktop game" OFF)
if(MC_BUILD_CLIENT)
    enable_language(C)
    add_subdirectory(client)
endif()

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
    add_executable(client_ore_tests client/tests/ore_tests.cpp client/src/ores.cpp)
    target_compile_features(client_ore_tests PRIVATE cxx_std_17)
    target_include_directories(client_ore_tests PRIVATE client/include)
    add_test(NAME client_ore_tests COMMAND client_ore_tests)
    add_executable(client_generation_tests client/tests/generation_tests.cpp
        client/src/generation.cpp client/src/imported_noise.cpp client/src/imported_blocks.cpp
        client/src/imported_terrain.cpp client/src/imported_decorator.cpp)
    target_compile_features(client_generation_tests PRIVATE cxx_std_17)
    target_include_directories(client_generation_tests PRIVATE client/include)
    add_test(NAME client_generation_tests COMMAND client_generation_tests)
    add_executable(client_language_tests client/tests/language_tests.cpp
        client/src/language.cpp client/src/items.cpp client/src/creative_tabs.cpp)
    target_compile_features(client_language_tests PRIVATE cxx_std_17)
    target_include_directories(client_language_tests PRIVATE client/include)
    add_test(NAME client_language_tests COMMAND client_language_tests)
    add_executable(client_entity_tests client/tests/entity_tests.cpp client/src/mobs.cpp client/src/items.cpp client/src/language.cpp)
    target_compile_features(client_entity_tests PRIVATE cxx_std_17)
    target_include_directories(client_entity_tests PRIVATE client/include)
    add_test(NAME client_entity_tests COMMAND client_entity_tests)
    add_executable(client_animation_tests client/tests/animation_tests.cpp)
    target_compile_features(client_animation_tests PRIVATE cxx_std_17)
    target_include_directories(client_animation_tests PRIVATE client/include)
    add_test(NAME client_animation_tests COMMAND client_animation_tests)
endif()

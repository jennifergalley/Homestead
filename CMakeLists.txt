cmake_minimum_required(VERSION 3.16)
project(HomesteadSimulation LANGUAGES CXX)

if(MSVC)
    string(REGEX REPLACE "/EH[a-zA-Z-]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

add_library(HomesteadSimulation STATIC
    Source/SurvivalGame/Simulation/HomesteadRegionalGeneration.cpp
    Source/SurvivalGame/Simulation/HomesteadRegionalDescriptorCache.cpp
    Source/SurvivalGame/Simulation/HomesteadRegionalTerrainAdapter.cpp
    Source/SurvivalGame/Simulation/HomesteadWorldGeneration.cpp
    Source/SurvivalGame/Simulation/HomesteadSimulation.cpp
    Source/SurvivalGame/Simulation/HomesteadEstate.cpp)
target_include_directories(HomesteadSimulation PUBLIC Source/SurvivalGame/Simulation)
target_compile_features(HomesteadSimulation PUBLIC cxx_std_17)
set_target_properties(HomesteadSimulation PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(HomesteadSimulation PRIVATE /W4 /EHs-c-)
    target_compile_definitions(HomesteadSimulation PUBLIC _HAS_EXCEPTIONS=0)
else()
    target_compile_options(HomesteadSimulation PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()

enable_testing()
add_executable(HomesteadSimulationTests Tests/HomesteadSimulationTests.cpp)
target_link_libraries(HomesteadSimulationTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadSimulationTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadSimulationTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadSimulationTests COMMAND HomesteadSimulationTests)

add_executable(HomesteadWorldGenerationTests Tests/HomesteadWorldGenerationTests.cpp)
target_link_libraries(HomesteadWorldGenerationTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadWorldGenerationTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadWorldGenerationTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadWorldGenerationTests COMMAND HomesteadWorldGenerationTests)

add_executable(HomesteadRegionalGenerationTests Tests/HomesteadRegionalGenerationTests.cpp)
target_link_libraries(HomesteadRegionalGenerationTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadRegionalGenerationTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadRegionalGenerationTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadRegionalGenerationTests COMMAND HomesteadRegionalGenerationTests)

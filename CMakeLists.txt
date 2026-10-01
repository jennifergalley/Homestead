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
    Source/SurvivalGame/Simulation/HomesteadBed.cpp
    Source/SurvivalGame/Simulation/HomesteadCrops.cpp
    Source/SurvivalGame/Simulation/HomesteadFood.cpp
    Source/SurvivalGame/Simulation/HomesteadCalendar.cpp
    Source/SurvivalGame/Simulation/HomesteadCalendarHooks.cpp
    Source/SurvivalGame/Simulation/HomesteadOvergrowth.cpp
    Source/SurvivalGame/Simulation/HomesteadEstate.cpp
    Source/SurvivalGame/Simulation/HomesteadEstateDisrepair.cpp
    Source/SurvivalGame/Simulation/HomesteadParcels.cpp
    Source/SurvivalGame/Simulation/HomesteadItems.cpp
    Source/SurvivalGame/Simulation/HomesteadShops.cpp
    Source/SurvivalGame/Simulation/HomesteadRain.cpp
    Source/SurvivalGame/Simulation/HomesteadManor.cpp
    Source/SurvivalGame/Simulation/HomesteadLamp.cpp
    Source/SurvivalGame/Simulation/HomesteadLampLight.cpp
    Source/SurvivalGame/Simulation/HomesteadRoomAudio.cpp
    Source/SurvivalGame/Simulation/HomesteadDoor.cpp
    Source/SurvivalGame/Simulation/HomesteadChests.cpp
    Source/SurvivalGame/Simulation/HomesteadBackpack.cpp
    Source/SurvivalGame/Simulation/HomesteadNightLight.cpp
    Source/SurvivalGame/Simulation/HomesteadPackRow.cpp
    Source/SurvivalGame/Simulation/HomesteadEstatePublicRoad.cpp
    Source/SurvivalGame/Simulation/HomesteadTravel.cpp
    Source/SurvivalGame/Simulation/HomesteadGardenTarget.cpp
    Source/SurvivalGame/Simulation/HomesteadEstateCoveRoute.cpp
    Source/SurvivalGame/Simulation/HomesteadCoveRouteKit.cpp)
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

add_executable(HomesteadParcelTests Tests/HomesteadParcelTests.cpp)
target_link_libraries(HomesteadParcelTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadParcelTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadParcelTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadParcelTests COMMAND HomesteadParcelTests)

add_executable(HomesteadEconomyTests Tests/HomesteadEconomyTests.cpp)
target_link_libraries(HomesteadEconomyTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadEconomyTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadEconomyTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadEconomyTests COMMAND HomesteadEconomyTests)

add_executable(HomesteadMapGeometryTests Tests/HomesteadMapGeometryTests.cpp Source/SurvivalGame/UI/HomesteadMapGeometry.cpp)
target_include_directories(HomesteadMapGeometryTests PRIVATE Source/SurvivalGame/UI)
target_compile_features(HomesteadMapGeometryTests PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(HomesteadMapGeometryTests PRIVATE /W4 /EHs-c-)
    target_compile_definitions(HomesteadMapGeometryTests PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(HomesteadMapGeometryTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadMapGeometryTests COMMAND HomesteadMapGeometryTests)

add_executable(HomesteadTextEditTests Tests/HomesteadTextEditTests.cpp)
target_include_directories(HomesteadTextEditTests PRIVATE Source/SurvivalGame/UI)
target_compile_features(HomesteadTextEditTests PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(HomesteadTextEditTests PRIVATE /W4 /EHs-c-)
    target_compile_definitions(HomesteadTextEditTests PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(HomesteadTextEditTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadTextEditTests COMMAND HomesteadTextEditTests)

add_executable(HomesteadHudTimingTests Tests/HomesteadHudTimingTests.cpp)
target_include_directories(HomesteadHudTimingTests PRIVATE Source/SurvivalGame/UI)
target_compile_features(HomesteadHudTimingTests PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(HomesteadHudTimingTests PRIVATE /W4 /EHs-c-)
    target_compile_definitions(HomesteadHudTimingTests PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(HomesteadHudTimingTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadHudTimingTests COMMAND HomesteadHudTimingTests)

add_executable(HomesteadManorTests Tests/HomesteadManorTests.cpp)
target_link_libraries(HomesteadManorTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadManorTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadManorTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadManorTests COMMAND HomesteadManorTests)

add_executable(HomesteadChestTests Tests/HomesteadChestTests.cpp)
target_link_libraries(HomesteadChestTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadChestTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadChestTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadChestTests COMMAND HomesteadChestTests)

add_executable(HomesteadLampTests Tests/HomesteadLampTests.cpp)
target_link_libraries(HomesteadLampTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadLampTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadLampTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadLampTests COMMAND HomesteadLampTests)

add_executable(HomesteadCalendarTests Tests/HomesteadCalendarTests.cpp)
target_link_libraries(HomesteadCalendarTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadCalendarTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadCalendarTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadCalendarTests COMMAND HomesteadCalendarTests)

add_executable(HomesteadPublicRoadTests Tests/HomesteadPublicRoadTests.cpp)
target_link_libraries(HomesteadPublicRoadTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadPublicRoadTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadPublicRoadTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadPublicRoadTests COMMAND HomesteadPublicRoadTests)

add_executable(HomesteadCoveRouteTests Tests/HomesteadCoveRouteTests.cpp)
target_link_libraries(HomesteadCoveRouteTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadCoveRouteTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadCoveRouteTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadCoveRouteTests COMMAND HomesteadCoveRouteTests)

add_executable(HomesteadPackRowTests Tests/HomesteadPackRowTests.cpp)
target_link_libraries(HomesteadPackRowTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadPackRowTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadPackRowTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadPackRowTests COMMAND HomesteadPackRowTests)

add_executable(HomesteadRoomTests Tests/HomesteadRoomTests.cpp)
target_link_libraries(HomesteadRoomTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadRoomTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadRoomTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadRoomTests COMMAND HomesteadRoomTests)

add_executable(HomesteadMusicPlaylistTests Tests/HomesteadMusicPlaylistTests.cpp)
target_link_libraries(HomesteadMusicPlaylistTests PRIVATE HomesteadSimulation)
if(MSVC)
    target_compile_options(HomesteadMusicPlaylistTests PRIVATE /W4 /WX /EHs-c-)
else()
    target_compile_options(HomesteadMusicPlaylistTests PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()
add_test(NAME HomesteadMusicPlaylistTests COMMAND HomesteadMusicPlaylistTests)

add_executable(HomesteadNightLightTests Tests/HomesteadNightLightTests.cpp)
target_link_libraries(HomesteadNightLightTests PRIVATE HomesteadSimulation)
# The test also checks the world's console-variable defaults against the tuned schedule.
target_compile_definitions(HomesteadNightLightTests PRIVATE HOMESTEAD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
if(MSVC)
    target_compile_options(HomesteadNightLightTests PRIVATE /W4 /EHs-c-)
else()
    target_compile_options(HomesteadNightLightTests PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)
endif()
add_test(NAME HomesteadNightLightTests COMMAND HomesteadNightLightTests)

add_executable(HomesteadAudioLevelTests Tests/HomesteadAudioLevelTests.cpp)
target_link_libraries(HomesteadAudioLevelTests PRIVATE HomesteadSimulation)
target_compile_definitions(HomesteadAudioLevelTests PRIVATE HOMESTEAD_REPO_ROOT="${CMAKE_SOURCE_DIR}")
if(MSVC)
    target_compile_options(HomesteadAudioLevelTests PRIVATE /W4 /WX /EHs-c-)
else()
    target_compile_options(HomesteadAudioLevelTests PRIVATE -Wall -Wextra -Wpedantic -Werror -fno-exceptions)
endif()
add_test(NAME HomesteadAudioLevelTests COMMAND HomesteadAudioLevelTests)

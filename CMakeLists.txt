cmake_minimum_required(VERSION 3.20)
project(YoshiIsland C CXX)
set(CMAKE_C_STANDARD 11)
set(SNESRECOMP_ROOT "${CMAKE_SOURCE_DIR}/upstream/snesrecomp")
include(${SNESRECOMP_ROOT}/runner/runner.cmake)
add_executable(YoshiIsland ${SNESRECOMP_RUNNER_SOURCES}
  src/main.c src/game_rtl.c src/host_contract.c)
snesrecomp_rom_identity(YoshiIsland ${CMAKE_SOURCE_DIR}/rom_identity.txt)
snesrecomp_target_generated_code(YoshiIsland ${CMAKE_SOURCE_DIR}/generated/usa11/generated)
target_include_directories(YoshiIsland PRIVATE ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
  ${SNESRECOMP_ROOT}/runner/src/desktop ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/generated/usa11/config)
target_link_libraries(YoshiIsland PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
snesrecomp_target_sdl(YoshiIsland)
snesrecomp_target_desktop_host(YoshiIsland)

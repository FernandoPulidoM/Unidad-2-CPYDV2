cmake_minimum_required(VERSION 3.31.6)
project(Tournaments)
#
# Asegúrate de que CMAKE_PREFIX_PATH incluya vcpkg
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux/share" ${CMAKE_PREFIX_PATH})

find_package(GTest REQUIRED CONFIG)
#
cmake_policy(SET CMP0135 NEW)

set(CMAKE_CXX_STANDARD 23)

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(activemq-cpp CONFIG REQUIRED)

add_subdirectory(tournament_common)
add_subdirectory(tournament_services)
add_subdirectory(tournament_consumer)

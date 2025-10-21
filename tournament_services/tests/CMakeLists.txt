cmake_minimum_required(VERSION 3.20)

project(tournament_tests CXX)
enable_testing()

# ---- SOLO tests de tournaments por ahora ----
set(TEST_SOURCES
        controller/TournamentControllerTest.cpp
        delegate/TournamentDelegateTest.cpp

        controller/TeamControllerTest.cpp
        delegate/TeamDelegateTest.cpp

        #        delegate/GroupDelegateTest.cpp


        # fuentes de producción necesarias por estos tests
        ../src/controller/TournamentController.cpp
        ../src/delegate/TournamentDelegate.cpp
        ../src/controller/TeamController.cpp
        ../src/delegate/TeamDelegate.cpp
        mocks/TeamDelegateMock.hpp
        mocks/TeamRepositoryMock.h
        #        ../src/controller/GroupController.cpp




)

# Ejecutable de tests
add_executable(${PROJECT_NAME}_runner ${TEST_SOURCES})

# Includes que necesitan los tests
target_include_directories(${PROJECT_NAME}_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tournament_common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
        ${CMAKE_SOURCE_DIR}/tournament_services/include

)

# Dependencias
find_package(GTest CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        Crow::Crow
        nlohmann_json::nlohmann_json
        tournament_common
)

# Registrar los tests
add_test(NAME AllTestsInMain COMMAND ${PROJECT_NAME}_runner)

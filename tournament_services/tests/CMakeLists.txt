project(tournament_services_tests)

set(CMAKE_CXX_STANDARD 23)

enable_testing()

find_package(GTest CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

include_directories(
        ../include              # headers de controllers / delegates
        ../../tournament_common/include
)

# Ejecutable de pruebas
add_executable(tournament_tests_runner
        # Tests de controllers
        controller/TournamentControllerTest.cpp
        controller/TeamControllerTest.cpp
        controller/GroupControllerTest.cpp
        controller/MatchControllerTest.cpp

        # Tests de delegates
        delegate/TournamentDelegateTest.cpp
        delegate/TeamDelegateTest.cpp
        delegate/GroupDelegateTest.cpp
        delegate/MatchDelegateTest.cpp

        # Mocks
        mocks/TeamRepositoryMock.h
        mocks/GroupDelegateMock.hpp
        mocks/MatchRepositoryMock.hpp
        mocks/MatchStrategyMock.hpp
        mocks/IMatchDelegateMock.hpp

        # FUENTES REALES QUE IMPLEMENTAN LOS METODOS LLAMADOS EN LOS TESTS
        ../src/controller/TournamentController.cpp
        ../src/controller/TeamController.cpp
        ../src/controller/GroupController.cpp
        ../src/controller/MatchController.cpp

        ../src/delegate/TournamentDelegate.cpp
        ../src/delegate/TeamDelegate.cpp
)

target_link_libraries(tournament_tests_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main

        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common
)

add_test(NAME tournament_tests_runner
        COMMAND tournament_tests_runner)

project(tournament_tests)

set(TEST_SOURCES
        controller/TournamentControllerTest.cpp
        controller/TeamControllerTest.cpp
        delegate/TournamentDelegateTest.cpp
        TournamentCrudTest.cpp
)

set(SOURCES ${TEST_SOURCES})

# Incluye los directorios de headers
include_directories(../include)
include_directories(../../tournament_common/include)

# Encuentra Google Test
find_package(GTest CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
#find_package(activemq-cpp CONFIG REQUIRED)

# Crea el ejecutable de tests
add_executable(${PROJECT_NAME}_runner
        ${TEST_SOURCES}
)

# Linkea las librerías necesarias
target_link_libraries(${PROJECT_NAME}_runner PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        tournament_common
        tournament_services_lib
        Crow::Crow
        libpqxx::pqxx
        nlohmann_json::nlohmann_json
        activemq-cpp
)

# Registra los tests
enable_testing()
add_test(NAME AllTests COMMAND ${PROJECT_NAME}_runner)
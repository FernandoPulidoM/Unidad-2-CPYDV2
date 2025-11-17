cmake_minimum_required(VERSION 3.22)
project(tournament_consumer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fuentes del consumer
set(CONSUMER_SOURCES
        src/processors/TeamAddedEventProcessor.cpp
        src/processors/ScoreRecordedEventProcessor.cpp
        main.cpp
        ../tournament_services/tests/mocks/MatchRepositoryMock.hpp
        ../tournament_services/tests/delegate/MatchDelegateTest.cpp
        ../tournament_common/include/persistence/repository/IMatchRepository.hpp
)

find_package(libpqxx CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")

# Opcional: Crow/asio si tu proyecto ya los trae en superbuild
# Si no existen targets, quitalos de target_link_libraries.
# find_package(crow CONFIG QUIET)
# find_package(asio CONFIG QUIET)

add_executable(${PROJECT_NAME} ${CONSUMER_SOURCES})

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/tournament_consumer/include
        ${CMAKE_SOURCE_DIR}/tournament_common/include
        ${HYPODERMIC_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common
        Crow::Crow
        asio::asio
)

# Copia el configuration.json del consumer al binario
configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
        COPYONLY
)

project(tournament_services)

set(CMAKE_CXX_STANDARD 23)

# Agregamos MatchDelegate y sus headers a la lista
set(SERVICES_SOURCES
        src/delegate/TeamDelegate.cpp
        src/delegate/TournamentDelegate.cpp
        src/delegate/MatchDelegate.cpp
        src/controller/TournamentController.cpp
        src/controller/TeamController.cpp
        src/controller/GroupController.cpp
        src/controller/MatchController.cpp
        src/delegate/TournamentRepository.cpp
        include/persistence/TournamentRepository.hpp
        include/delegate/MatchDelegate.hpp
        include/delegate/IMatchDelegate.hpp
)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(tests)

include_directories(include)

add_executable(${PROJECT_NAME}
        main.cpp
        ${SERVICES_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common
)

target_include_directories(${PROJECT_NAME} INTERFACE ${HYPODERMIC_INCLUDE_DIRS})

configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
        COPYONLY
)

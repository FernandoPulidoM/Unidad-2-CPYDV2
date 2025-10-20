project(tournament_services)

set(CMAKE_CXX_STANDARD 23)

set(SERVICES_SOURCES
        src/delegate/TeamDelegate.cpp
        src/delegate/TournamentDelegate.cpp
        src/controller/TournamentController.cpp
        src/controller/TeamController.cpp
        src/delegate/TournamentRepository.cpp
        include/persistence/TournamentRepository.hpp)

include(CTest)
enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_path(HYPODERMIC_INCLUDE_DIRS "Hypodermic/ActivatedRegistrationInfo.h")
find_package(nlohmann_json CONFIG REQUIRED)
#find_package(activemq-cpp CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

include_directories(include)

# Crea una librería con el código de services (sin main)
add_library(tournament_services_lib
        ${SERVICES_SOURCES}
)

target_link_libraries(tournament_services_lib PUBLIC
        Crow::Crow
        asio::asio
        nlohmann_json::nlohmann_json
        libpqxx::pqxx
        unofficial::activemq-cpp::activemq-cpp
        tournament_common)

target_include_directories(tournament_services_lib INTERFACE ${HYPODERMIC_INCLUDE_DIRS})

# El ejecutable principal linkea la librería
add_executable(${PROJECT_NAME}
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC tournament_services_lib)

# Subdirectorio de tests
add_subdirectory(tests)

configure_file(
        ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/configuration.json
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/configuration.json
        COPYONLY)
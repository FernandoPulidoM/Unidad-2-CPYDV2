#ifndef RESTAPI_CONTAINER_SETUP_HPP
#define RESTAPI_CONTAINER_SETUP_HPP

#include <Hypodermic/Hypodermic.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <memory>

// ===== Config & infra =====
#include "RunConfiguration.hpp"
#include "persistence/configuration/PostgresConnectionProvider.hpp"
#include "cms/ConnectionManager.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/QueueResolver.hpp"

// ===== Repos =====
#include "persistence/repository/IRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"        // persistence::TeamRepository
#include "persistence/repository/TournamentRepository.hpp"  // (parece estar en global)
#include "persistence/repository/GroupRepository.hpp"       // (parece estar en global)
#include "persistence/repository/MatchRepository.hpp"       // persistence::MatchRepository

// ===== Delegates =====
#include "delegate/TeamDelegate.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "delegate/GroupDelegate.hpp"
#include "delegate/IMatchDelegate.hpp"
#include "delegate/MatchDelegate.hpp"

// ===== Controllers =====
#include "controller/TeamController.hpp"
#include "controller/TournamentController.hpp"
#include "controller/GroupController.hpp"
#include "controller/MatchController.hpp"

namespace config {

inline std::shared_ptr<Hypodermic::Container> containerSetup() {
    Hypodermic::ContainerBuilder builder;

    // --- Configuracion ---
    std::ifstream file("configuration.json");
    nlohmann::json configuration;
    file >> configuration;

    auto appConfig = std::make_shared<RunConfiguration>(configuration["runConfig"]);
    builder.registerInstance(appConfig);

    auto postgressConnection = std::make_shared<PostgresConnectionProvider>(
        configuration["databaseConfig"]["connectionString"].get<std::string>(),
        configuration["databaseConfig"]["poolSize"].get<size_t>()
    );
    builder.registerInstance(postgressConnection).as<IDbConnectionProvider>();

    // --- ActiveMQ ---
    builder.registerType<ConnectionManager>()
        .onActivated([configuration](Hypodermic::ComponentContext&, const std::shared_ptr<ConnectionManager>& instance) {
            instance->initialize(configuration["activemq"]["broker-url"].get<std::string>());
        })
        .singleInstance();

    builder.registerType<QueueMessageProducer>().named("tournamentAddTeamQueue");

    builder.registerType<QueueResolver>()
        .as<IResolver<IQueueMessageProducer>>()
        .named("queueResolver")
        .singleInstance();

    // --- Repos ---
    // TeamRepository SI esta en namespace persistence
    builder.registerType<persistence::TeamRepository>()
        .as<IRepository<domain::Team, std::string_view>>()   // interfaz en global
        .singleInstance();

    // GroupRepository y TournamentRepository aparentan estar en el global namespace
    builder.registerType<GroupRepository>()
        .as<IGroupRepository>()
        .singleInstance();

    builder.registerType<TournamentRepository>()
        .as<IRepository<domain::Tournament, std::string>>()
        .singleInstance();

    // MatchRepository sin interfaz: instancia concreta
    builder.registerInstance(std::make_shared<persistence::MatchRepository>(postgressConnection));

    // --- Delegates ---
    builder.registerType<TeamDelegate>()
        .as<ITeamDelegate>()
        .singleInstance();

    builder.registerType<GroupDelegate>()
        .as<IGroupDelegate>()
        .singleInstance();

    builder.registerType<TournamentDelegate>()
        .as<ITournamentDelegate>()
        .singleInstance();

    builder.registerType<services::MatchDelegate>()
        .as<IMatchDelegate>()
        .singleInstance();

    // --- Controllers ---
    builder.registerType<TeamController>().singleInstance();
    builder.registerType<GroupController>().singleInstance();
    builder.registerType<TournamentController>().singleInstance();
    builder.registerType<services::MatchController>().singleInstance();

    return builder.build();
}

} // namespace config
#endif // RESTAPI_CONTAINER_SETUP_HPP

//-- podman run -d --replace --name=tournament_db --network dev -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
//-- podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql

#include <activemq/library/ActiveMQCPP.h>

#include "configuration/RouteDefinition.hpp"
#include "configuration/ContainerSetup.hpp"
#include "configuration/RunConfiguration.hpp"
#include "controller/GroupController.hpp"
#include "controller/MatchController.hpp"
#include "delegate/MatchDelegate.hpp"
#include "../tournament_common/include/domain/RoundRobinStragety.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();

    // Configurar el contenedor DI
    const auto container = config::containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<config::RunConfiguration>();

    // Resolver dependencias desde el contenedor
    auto dbProvider = container->resolve<IDbConnectionProvider>();

    // Crear repositorios
    auto matchRepo = std::make_shared<persistence::MatchRepository>(dbProvider);
    auto groupRepo = container->resolve<IGroupRepository>();

    // Crear estrategia y delegate
    auto matchStrategy = std::make_shared<RoundRobinStrategy>();
    auto matchDelegate = std::make_shared<MatchDelegate>(
        matchRepo,
        groupRepo,
        matchStrategy
    );

    // Crear controlador (si no está registrado automáticamente)
    auto matchController = std::make_shared<services::MatchController>(matchDelegate);

    // Si MatchController NO está en el registro automático, registra sus rutas manualmente:
    // (Esto solo es necesario si las macros REGISTER_ROUTE no funcionaron)
    /*
    CROW_ROUTE(app, "/tournaments/<string>/matches")
        .methods("GET"_method)
        ([matchController](const crow::request& req, const std::string& tid) {
            return matchController->GetMatches(req, tid);
        });

    CROW_ROUTE(app, "/tournaments/<string>/matches/<string>")
        .methods("GET"_method)
        ([matchController](const crow::request& req, const std::string& tid, const std::string& mid) {
            return matchController->GetMatch(req, tid, mid);
        });

    CROW_ROUTE(app, "/tournaments/<string>/matches/<string>")
        .methods("PATCH"_method)
        ([matchController](const crow::request& req, const std::string& tid, const std::string& mid) {
            return crow::response(matchController->PatchScore(req, tid, mid));
        });

    CROW_ROUTE(app, "/tournaments/<string>/matches/generate")
        .methods("POST"_method)
        ([matchController](const crow::request& req, const std::string& tid) {
            return crow::response(matchController->GenerateMatches(req, tid));
        });
    */

    app.port(appConfig->port)
        .concurrency(appConfig->concurrency)
        .run();

    activemq::library::ActiveMQCPP::shutdownLibrary();

    return 0;
}
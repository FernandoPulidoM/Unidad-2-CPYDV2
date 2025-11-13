//-- podman run -d --replace --name=tournament_db --network dev -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
//-- podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql

#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/RouteDefinition.hpp"
#include "configuration/ContainerSetup.hpp"
#include "controller/GroupController.hpp"
#include "include/configuration/ContainerSetup.hpp"
#include "include/configuration/RunConfiguration.hpp"
#include "../tournament_common/include/cms/QueueMessageConsumer.hpp"
#include "../tournament_common/include/domain/TournamentMatchListener.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();

    const auto container = config::containerSetup();

    // ========================================
    // INICIAR LISTENER EN THREAD SEPARADO
    // ========================================

    // ✅ CORRECCIÓN: Quitar el <std::shared_ptr<...>>
    auto connectionManager = container->resolve<ConnectionManager>();

    // Ahora connectionManager es std::shared_ptr<ConnectionManager>
    auto consumer = std::make_shared<QueueMessageConsumer>(connectionManager);
    auto listener = std::make_shared<TournamentMatchListener>(consumer);

    std::thread listenerThread([listener]() {
        listener->Start();
    });

    // ========================================
    // CONFIGURAR SERVIDOR HTTP (CROW)
    // ========================================
    crow::SimpleApp app;

    // Registrar todas las rutas
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<config::RunConfiguration>();

    std::cout << "Starting server on port " << appConfig->port
              << " with " << appConfig->concurrency << " threads" << std::endl;

    // Iniciar servidor HTTP (bloquea hasta Ctrl+C)
    app.port(appConfig->port)
        .concurrency(appConfig->concurrency)
        .run();

    // ========================================
    // LIMPIEZA AL TERMINAR
    // ========================================
    std::cout << "Shutting down..." << std::endl;

    consumer->Stop();

    if (listenerThread.joinable()) {
        listenerThread.join();
    }

    activemq::library::ActiveMQCPP::shutdownLibrary();

    return 0;
}
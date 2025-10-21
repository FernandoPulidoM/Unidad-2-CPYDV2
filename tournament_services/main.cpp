//-- podman run -d --replace --name=tournament_db --network dev -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
//-- podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql

//

#include <activemq/library/ActiveMQCPP.h>

#include "configuration/RouteDefinition.hpp"
#include "configuration/ContainerSetup.hpp"
#include "controller/GroupController.hpp"
#include "include/configuration/ContainerSetup.hpp"
#include "include/configuration/RunConfiguration.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    const auto container = config::containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<config::RunConfiguration>();

    app.port(appConfig->port)
        .concurrency(appConfig->concurrency)
        .run();
    activemq::library::ActiveMQCPP::shutdownLibrary();
}

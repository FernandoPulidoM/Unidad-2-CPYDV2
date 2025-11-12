//
// Consumer main
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <chrono>
#include <print>   // C++23 std::println

#include "configuration/ContainerSetup.hpp"

// Nota:
// Tu QueueMessageConsumer ya se resuelve desde el contenedor.
// Aqui solo arrancamos listeners para las colas/topics que nos interesan.

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        // Listener para eventos ya existentes
        std::thread tTournamentCreated([&] {
            auto listener = container->resolve<QueueMessageConsumer>();
            listener->Start("tournament.created");
        });

        // NUEVO: se generan partidos RR cuando se agregan equipos
        std::thread tTeamAdded([&] {
            auto listener = container->resolve<QueueMessageConsumer>();
            listener->Start("group.team_added");      // o "tournament.team_added" si asi lo emites
        });

        // NUEVO: al registrar marcador, avanzar fase / crear llaves
        std::thread tScoreRecorded([&] {
            auto listener = container->resolve<QueueMessageConsumer>();
            listener->Start("match.score_recorded");
        });

        // Mantener el proceso vivo
        tTournamentCreated.detach();
        tTeamAdded.detach();
        tScoreRecorded.detach();

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}

#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <chrono>
#include <print>

#include "configuration/ContainerSetup.hpp"
#include "cms/QueueMessageConsumer.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        const auto container = config::containerSetup();

        // listeners
        std::thread tTournamentCreated([&] {
            auto q = container->resolve<QueueMessageConsumer>();
            q->Start("tournament.created");
        });

        std::thread tTeamAdded([&] {
            auto q = container->resolve<QueueMessageConsumer>();
            q->Start("group.team_added");
        });

        std::thread tScoreRecorded([&] {
            auto q = container->resolve<QueueMessageConsumer>();
            q->Start("match.score_recorded");
        });

        tTournamentCreated.detach();
        tTeamAdded.detach();
        tScoreRecorded.detach();

        for (;;) std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}

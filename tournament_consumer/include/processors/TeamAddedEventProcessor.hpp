#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// FW decls para no incluir headers pesados aqui
namespace persistence {
    class MatchRepository;
    class TeamRepository;
}

namespace consumers {

    class TeamAddedEventProcessor {
    public:
        // ctor simple (Hypodermic puede autowirear solo por tipo)
        TeamAddedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                std::shared_ptr<persistence::TeamRepository> teamRepo)
            : matchRepo_(std::move(matchRepo)),
              teamRepo_(std::move(teamRepo)) {}

        // ctor completo opcional si quieres inyectar configs manualmente
        TeamAddedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                std::shared_ptr<persistence::TeamRepository> teamRepo,
                                int rrMinTeams,
                                int rrMatchPoints)
            : matchRepo_(std::move(matchRepo)),
              teamRepo_(std::move(teamRepo)),
              rrMinTeams_(rrMinTeams),
              rrMatchPoints_(rrMatchPoints) {}

        void process(const nlohmann::json& evt);

    private:
        void generateRoundRobinOnTheFly(const std::string& tid,
                                        const std::vector<std::string>& teamIds);

        std::shared_ptr<persistence::MatchRepository> matchRepo_;
        std::shared_ptr<persistence::TeamRepository>  teamRepo_;
        int rrMinTeams_{3};
        int rrMatchPoints_{2};
    };

} // namespace consumers

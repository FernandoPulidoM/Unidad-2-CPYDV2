#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "tournament_common/include/persistence/repository/MatchRepository.hpp"
#include "tournament_common/include/persistence/repository/TeamRepository.hpp"

namespace consumers {

    class TeamAddedEventProcessor {
    public:
        TeamAddedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                std::shared_ptr<persistence::TeamRepository> teamRepo)
          : matchRepo_(std::move(matchRepo)), teamRepo_(std::move(teamRepo)) {}

        // payload: { "tournamentId":"...", "teamId":"..." }
        void process(const nlohmann::json& evt);

    private:
        void generateRoundRobinOnTheFly(const std::string& tid,
                                        const std::vector<std::string>& teamIds);

        std::shared_ptr<persistence::MatchRepository> matchRepo_;
        std::shared_ptr<persistence::TeamRepository>  teamRepo_;
    };

} // namespace consumers

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "tournament_common/include/persistence/repository/MatchRepository.hpp"
#include "tournament_common/include/persistence/repository/TeamRepository.hpp"

namespace consumers {

    class ScoreRecordedEventProcessor {
    public:
        ScoreRecordedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                    std::shared_ptr<persistence::TeamRepository> teamRepo)
          : matchRepo_(std::move(matchRepo)), teamRepo_(std::move(teamRepo)) {}

        // payload: { "tournamentId":"...", "matchId":"...", "score": { "home":int, "visitor":int } }
        void process(const nlohmann::json& evt);

    private:
        struct Row { std::string teamId; int wins=0, pf=0, pa=0; };
        bool allRegularPlayed(const std::string& tid);
        std::vector<Row> computeRoundRobinTable(const std::string& tid);
        void createEliminationBracketTopN(const std::string& tid, int topN);

        std::shared_ptr<persistence::MatchRepository> matchRepo_;
        std::shared_ptr<persistence::TeamRepository>  teamRepo_;
    };

} // namespace consumers

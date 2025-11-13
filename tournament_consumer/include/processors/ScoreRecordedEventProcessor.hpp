#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace persistence {
    class MatchRepository;  // fwd
    class TeamRepository;   // fwd
}

namespace consumers {

    class ScoreRecordedEventProcessor {
    public:
        // ctor simple para autowire
        ScoreRecordedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                    std::shared_ptr<persistence::TeamRepository>  teamRepo)
            : matchRepo_(std::move(matchRepo)),
              teamRepo_(std::move(teamRepo)) {}

        // ctor completo opcional
        ScoreRecordedEventProcessor(std::shared_ptr<persistence::MatchRepository> matchRepo,
                                    std::shared_ptr<persistence::TeamRepository>  teamRepo,
                                    int rrMinTeams,
                                    int rrMatchPoints,
                                    int minRegularToPlayoffs)
            : matchRepo_(std::move(matchRepo)),
              teamRepo_(std::move(teamRepo)),
              rrMinTeams_(rrMinTeams),
              rrMatchPoints_(rrMatchPoints),
              minRegularToPlayoffs_(minRegularToPlayoffs) {}

        void process(const nlohmann::json& evt);

    private:
        struct Row {
            std::string teamId;
            int played{0};
            int wins{0};
            int draws{0};
            int losses{0};
            int points{0};
        };

        bool allRegularPlayed(const std::string& tid);
        std::vector<Row> computeRoundRobinTable(const std::string& tid);
        void maybeCreatePlayoffs(const std::string& tid, const std::vector<Row>& table);

        std::shared_ptr<persistence::MatchRepository> matchRepo_;
        std::shared_ptr<persistence::TeamRepository>  teamRepo_;
        int rrMinTeams_{3};
        int rrMatchPoints_{2};
        int minRegularToPlayoffs_{4}; // usado por el .cpp
    };

} // namespace consumers

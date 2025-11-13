#include "processors/TeamAddedEventProcessor.hpp"

// Agrega estos 2 includes para materializar los tipos forward-declarados
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"

#include <nlohmann/json.hpp>
using nlohmann::json;

namespace consumers {

    void TeamAddedEventProcessor::process(const json& evt) {
        const std::string tid = evt.value("tournamentId", "");
        if (tid.empty()) return;

        auto teams = teamRepo_->ListByTournament(tid);  // ahora compila

        std::vector<std::string> teamIds;
        teamIds.reserve(teams.size());
        for (const auto& t : teams) teamIds.push_back(std::string(t.Id));

        if ((int)teamIds.size() >= rrMinTeams_) {
            generateRoundRobinOnTheFly(tid, teamIds);
        }
    }

    void TeamAddedEventProcessor::generateRoundRobinOnTheFly(
        const std::string& tid,
        const std::vector<std::string>& teamIds
    ) {
        std::vector<nlohmann::json> docs;
        for (size_t i = 0; i < teamIds.size(); ++i) {
            for (size_t j = i + 1; j < teamIds.size(); ++j) {
                nlohmann::json doc = {
                    {"tournamentId", tid},
                    {"homeTeamId",   teamIds[i]},
                    {"awayTeamId",   teamIds[j]},
                    {"round",        "regular"},
                    {"status",       "scheduled"},
                    {"score",        nullptr}
                };
                docs.push_back(std::move(doc));
            }
        }
        (void) matchRepo_->CreateBulk(docs);  // ahora compila
    }

} // namespace consumers

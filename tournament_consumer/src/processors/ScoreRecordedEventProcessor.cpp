#include "processors/ScoreRecordedEventProcessor.hpp"

// Materializa forward-decls
#include "persistence/repository/TeamRepository.hpp"
#include "persistence/repository/MatchRepository.hpp"

using nlohmann::json;

namespace consumers {

    void ScoreRecordedEventProcessor::process(const json& evt) {
        const std::string tid = evt.value("tournamentId", "");
        const std::string mid = evt.value("matchId", "");
        const int h = evt["score"].value("home", 0);
        const int v = evt["score"].value("away", 0);

        (void) matchRepo_->UpdateScore(tid, mid, h, v);

        if (allRegularPlayed(tid)) {
            auto table = computeRoundRobinTable(tid);
            if ((int)table.size() >= minRegularToPlayoffs_) {
                maybeCreatePlayoffs(tid, table);
            }
        }
    }

    // --------- Implementaciones minimas para compilar ---------

    bool ScoreRecordedEventProcessor::allRegularPlayed(const std::string& /*tid*/) {
        // TODO: implementar contra MatchRepository si ya tienes API
        return false;
    }

    std::vector<ScoreRecordedEventProcessor::Row>
    ScoreRecordedEventProcessor::computeRoundRobinTable(const std::string& /*tid*/) {
        // TODO: implementar contra repos; por ahora regresa vacio para compilar
        return {};
    }

    void ScoreRecordedEventProcessor::maybeCreatePlayoffs(
        const std::string& /*tid*/,
        const std::vector<Row>& /*table*/
    ) {
        // TODO: implementar segun tu logica
    }

} // namespace consumers

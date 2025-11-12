#include "processors/TeamAddedEventProcessor.hpp"

using nlohmann::json;
using namespace domain;

namespace consumers {

    void TeamAddedEventProcessor::process(const json& evt) {
        const std::string tid = evt.at("tournamentId").get<std::string>();

        // Listar todos los equipos del torneo (ajusta a la firma real de tu repo)
        auto teams = teamRepo_->ListByTournament(tid);
        std::vector<std::string> ids;
        ids.reserve(teams.size());
        for (auto& t : teams) ids.push_back(t.id);

        generateRoundRobinOnTheFly(tid, ids);
    }

    void TeamAddedEventProcessor::generateRoundRobinOnTheFly(
        const std::string& tid, const std::vector<std::string>& teamIds) {

        std::vector<nlohmann::json> docs;
        for (size_t i = 0; i < teamIds.size(); ++i) {
            for (size_t j = i + 1; j < teamIds.size(); ++j) {
                const auto& a = teamIds[i];
                const auto& b = teamIds[j];
                if (matchRepo_->ExistsPairing(tid, a, b)) continue;

                nlohmann::json j = {
                    {"tournamentId", tid},
                    {"homeTeamId", a},
                    {"awayTeamId", b},
                    {"round", "regular"}
                };
                docs.push_back(std::move(j));
            }
        }
        if (!docs.empty()) (void)matchRepo_->CreateBulk(docs);
    }

} // namespace consumers

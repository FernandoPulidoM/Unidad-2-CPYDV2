#pragma once
#include <expected>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "tournament_common/include/domain/Match.hpp"
#include "tournament_common/include/persistence/configuration/IDbConnectionProvider.hpp"

namespace persistence {

    class MatchRepository {
    public:
        explicit MatchRepository(std::shared_ptr<configuration::IDbConnectionProvider> db)
          : db_(std::move(db)) {}

        // GET /tournaments/{tid}/matches?showMatches=played|pending
        std::vector<domain::Match>
        ListByTournament(const std::string& tournamentId,
                         const std::optional<std::string>& filterPlayedOrPending);

        // GET /tournaments/{tid}/matches/{mid}
        std::optional<domain::Match>
        GetById(const std::string& tournamentId, const std::string& matchId);

        // PATCH score
        std::expected<void, std::string>
        UpdateScore(const std::string& tournamentId, const std::string& matchId,
                    int home, int visitor);

        // Creacion y utilidades (para Round Robin y playoffs)
        bool ExistsPairing(const std::string& tournamentId,
                           const std::string& homeTeamId,
                           const std::string& awayTeamId);

        std::expected<std::string, std::string> Create(const nlohmann::json& docJson);
        std::expected<void, std::string> CreateBulk(const std::vector<nlohmann::json>& docs);

        // Publicador de eventos (conecta esto a tu producer real)
        void PublishEvent(const std::string& address, const std::string& jsonPayload);

    private:
        std::shared_ptr<configuration::IDbConnectionProvider> db_;
        static domain::Match fromRow(const std::string& id, const nlohmann::json& j);
    };

} // namespace persistence

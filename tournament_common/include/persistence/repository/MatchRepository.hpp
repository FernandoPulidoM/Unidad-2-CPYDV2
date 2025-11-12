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

        std::vector<domain::Match>
        ListByTournament(const std::string& tournamentId,
                         const std::optional<std::string>& filterPlayedOrPending);

        std::optional<domain::Match>
        GetById(const std::string& tournamentId, const std::string& matchId);

        std::expected<void, std::string>
        UpdateScore(const std::string& tournamentId, const std::string& matchId,
                    int home, int visitor);

        // helpers para generacion en consumidores
        bool ExistsPairing(const std::string& tournamentId,
                           const std::string& homeTeamId,
                           const std::string& awayTeamId);
        std::expected<std::string, std::string> Create(const domain::Match& m);
        std::expected<void, std::string> CreateBulk(const std::vector<domain::Match>& ms);

        // publicador minimalista de eventos (puedes redirigirlo a tu capa cms)
        void PublishEvent(const std::string& address, const std::string& jsonPayload);

    private:
        std::shared_ptr<configuration::IDbConnectionProvider> db_;
    };

} // namespace persistence

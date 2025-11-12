#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <expected>
#include "IMatchDelegate.hpp"
#include "tournament_common/include/persistence/repository/MatchRepository.hpp"
#include "tournament_common/include/persistence/repository/GroupRepository.hpp"
#include "tournament_common/include/persistence/repository/TeamRepository.hpp"
#include "tournament_common/include/persistence/repository/TournamentRepository.hpp"

namespace services {

    class MatchDelegate final : public IMatchDelegate {
    public:
        MatchDelegate(std::shared_ptr<persistence::MatchRepository> matchRepo,
                      std::shared_ptr<persistence::TournamentRepository> tourRepo,
                      std::shared_ptr<persistence::TeamRepository> teamRepo,
                      std::shared_ptr<persistence::GroupRepository> groupRepo)
            : matchRepo_(std::move(matchRepo)),
              tourRepo_(std::move(tourRepo)),
              teamRepo_(std::move(teamRepo)),
              groupRepo_(std::move(groupRepo)) {}

        std::vector<domain::Match>
        List(const std::string& tournamentId,
             const std::optional<std::string>& filterPlayedOrPending) override;

        std::optional<domain::Match>
        Get(const std::string& tournamentId, const std::string& matchId) override;

        std::expected<void, Error>
        UpdateScore(const std::string& tournamentId, const std::string& matchId,
                    int home, int visitor) override;

    private:
        bool isScoreValidRegular(int h, int v) const;  // 0..10, permite empate
        bool isScoreValidElimination(int h, int v) const; // 0..10, sin empate

        std::shared_ptr<persistence::MatchRepository> matchRepo_;
        std::shared_ptr<persistence::TournamentRepository> tourRepo_;
        std::shared_ptr<persistence::TeamRepository> teamRepo_;
        std::shared_ptr<persistence::GroupRepository> groupRepo_;
    };

} // namespace services

#pragma once
#include <memory>
#include "delegate/IMatchDelegate.hpp"
#include "persistence/repository/MatchRepository.hpp"

namespace services {

    // Implementacion concreta del delegate.
    class MatchDelegate : public IMatchDelegate {
    public:
        explicit MatchDelegate(std::shared_ptr<persistence::MatchRepository> repo)
            : repo_(std::move(repo)) {}

        std::vector<domain::Match>
        List(const std::string& tournamentId,
             const std::optional<std::string>& filter) override {
            return repo_->ListByTournament(tournamentId, filter);
        }

        std::optional<domain::Match>
        Get(const std::string& tournamentId, const std::string& matchId) override {
            return repo_->GetById(tournamentId, matchId);
        }

        std::expected<void, std::string>
        updateScore(const std::string& tournamentId, const std::string& matchId,
                    int home, int visitor) override {
            return repo_->UpdateScore(tournamentId, matchId, home, visitor);
        }

    private:
        std::shared_ptr<persistence::MatchRepository> repo_;
    };

} // namespace services

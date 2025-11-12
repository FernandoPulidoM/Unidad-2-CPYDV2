#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <optional>
#include <expected>

#include "IMatchDelegate.hpp"
#include "persistence/repository/MatchRepository.hpp"

class MatchDelegate : public IMatchDelegate {
public:
    explicit MatchDelegate(std::shared_ptr<persistence::MatchRepository> repo)
        : repo_(std::move(repo)) {}

    std::vector<domain::Match>
    List(const std::string& tid,
         const std::optional<std::string>& filter) override {
        return repo_->ListByTournament(tid, filter);
    }

    std::optional<domain::Match>
    Get(const std::string& tid, const std::string& mid) override {
        return repo_->GetById(tid, mid);
    }

    std::expected<void, std::string>
    updateScore(const std::string& tid,
                const std::string& mid,
                int home, int visitor) override {
        return repo_->UpdateScore(tid, mid, home, visitor);
    }

private:
    std::shared_ptr<persistence::MatchRepository> repo_;
};

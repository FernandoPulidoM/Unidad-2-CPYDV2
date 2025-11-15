#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <expected>
#include <iostream>

#include "delegate/IMatchDelegate.hpp"
#include "domain/IMatchStrategy.hpp"
#include "domain/Match.hpp"
#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include <nlohmann/json.hpp>

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<persistence::MatchRepository> matchRepo_;
    std::shared_ptr<IGroupRepository> groupRepo_;
    std::shared_ptr<IMatchStrategy> strategy_;

public:
    MatchDelegate(std::shared_ptr<persistence::MatchRepository> matchRepo,
                  std::shared_ptr<IGroupRepository> groupRepo,
                  std::shared_ptr<IMatchStrategy> strategy)
        : matchRepo_(std::move(matchRepo)),
          groupRepo_(std::move(groupRepo)),
          strategy_(std::move(strategy)) {}

    std::vector<domain::Match>
    List(const std::string& tournamentId,
         const std::optional<std::string>& filter) override {
        return matchRepo_->ListByTournament(tournamentId, filter);
    }

    std::optional<domain::Match>
    Get(const std::string& tournamentId, const std::string& matchId) override {
        return matchRepo_->GetById(tournamentId, matchId);
    }

    std::expected<void, std::string>
    updateScore(const std::string& tournamentId,
                const std::string& matchId, int home, int visitor) override {

        std::cout << "[MatchDelegate] Updating score for match " << matchId
                  << " in tournament " << tournamentId << std::endl;
        std::cout << "[MatchDelegate] Score: " << home << "-" << visitor << std::endl;

        auto match = matchRepo_->GetById(tournamentId, matchId);
        if (!match.has_value()) {
            std::cerr << "[MatchDelegate] Match not found!" << std::endl;
            return std::unexpected("Match not found");
        }

        std::cout << "[MatchDelegate] Match found, current status: "
                  << match->status << std::endl;

        if (match->status == "played") {
            std::cerr << "[MatchDelegate] Match already played!" << std::endl;
            return std::unexpected("Match already has a score");
        }

        auto result = matchRepo_->UpdateScore(tournamentId, matchId, home, visitor);

        if (result.has_value()) {
            std::cout << "[MatchDelegate] Score updated successfully!" << std::endl;
        } else {
            std::cerr << "[MatchDelegate] Failed to update: " << result.error() << std::endl;
        }

        return result;
    }

    std::expected<void, std::string>
    GenerateMatchesForTournament(const std::string& tournamentId) override {
        std::cout << "[MatchDelegate] Generating matches for tournament "
                  << tournamentId << std::endl;

        auto groups = groupRepo_->FindByTournamentId(tournamentId);
        if (groups.empty()) {
            std::cerr << "[MatchDelegate] No groups found!" << std::endl;
            return std::unexpected("No groups found for tournament");
        }

        std::cout << "[MatchDelegate] Found " << groups.size() << " groups" << std::endl;

        auto matches = strategy_->GenerateMatches(tournamentId, groups);
        if (!matches.has_value()) {
            std::cerr << "[MatchDelegate] Strategy failed: " << matches.error() << std::endl;
            return std::unexpected(matches.error());
        }

        std::cout << "[MatchDelegate] Generated " << matches->size() << " matches" << std::endl;

        std::vector<nlohmann::json> matchJsons;
        matchJsons.reserve(matches->size());

        for (const auto& match : *matches) {
            nlohmann::json j;
            j["tournamentId"] = match.tournamentId;
            j["homeTeamId"] = match.homeTeamId;
            j["awayTeamId"] = match.awayTeamId;
            j["round"] = match.round;
            j["status"] = match.status;
            j["score"] = nullptr;
            matchJsons.push_back(j);
        }

        auto result = matchRepo_->CreateBulk(matchJsons);

        if (result.has_value()) {
            std::cout << "[MatchDelegate] Matches saved successfully!" << std::endl;
        } else {
            std::cerr << "[MatchDelegate] Failed to save: " << result.error() << std::endl;
        }

        return result;
    }

    // NUEVO: Obtener estado del torneo
    std::expected<nlohmann::json, std::string>
    GetTournamentStatus(const std::string& tournamentId) override {
        try {
            auto matches = matchRepo_->ListByTournament(tournamentId, std::nullopt);

            int total = matches.size();
            int played = 0;
            int pending = 0;

            for (const auto& m : matches) {
                if (m.status == "played") played++;
                else pending++;
            }

            nlohmann::json status;
            status["totalMatches"] = total;
            status["playedMatches"] = played;
            status["pendingMatches"] = pending;
            status["groupStageComplete"] = (pending == 0 && total > 0);

            return status;
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Error getting status: ") + e.what());
        }
    }

    // STUB: Implementación básica de GetGroupStandings (para compilar)
    std::expected<std::vector<TeamStanding>, std::string>
    GetGroupStandings(const std::string& tournamentId, const std::string& groupId) override {
        // TODO: Implementar cálculo de tabla de posiciones
        return std::unexpected("Not implemented yet");
    }
};
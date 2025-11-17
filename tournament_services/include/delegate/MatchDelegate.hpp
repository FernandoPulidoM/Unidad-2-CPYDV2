#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <expected>
#include <iostream>
#include <map>
#include <algorithm>

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

    // ... (mantener métodos existentes: List, Get, updateScore) ...

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

        std::cout << "[MatchDelegate] Updating score for match " << matchId << std::endl;

        auto match = matchRepo_->GetById(tournamentId, matchId);
        if (!match.has_value()) {
            return std::unexpected("Match not found");
        }

        if (match->status == "played") {
            return std::unexpected("Match already has a score");
        }

        auto result = matchRepo_->UpdateScore(tournamentId, matchId, home, visitor);

        if (result.has_value()) {
            std::cout << "[MatchDelegate] Score updated successfully!" << std::endl;

            // Verificar si se completó una fase y avanzar automáticamente
            checkAndAdvancePhase(tournamentId, match->round);
        }

        return result;
    }

    std::expected<void, std::string>
    GenerateMatchesForTournament(const std::string& tournamentId) override {
        std::cout << "[MatchDelegate] Generating group stage matches" << std::endl;

        auto groups = groupRepo_->FindByTournamentId(tournamentId);
        if (groups.empty()) {
            return std::unexpected("No groups found for tournament");
        }

        auto matches = strategy_->GenerateMatches(tournamentId, groups);
        if (!matches.has_value()) {
            return std::unexpected(matches.error());
        }

        std::vector<nlohmann::json> matchJsons;
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

        return matchRepo_->CreateBulk(matchJsons);
    }

    std::expected<nlohmann::json, std::string>
    GetTournamentStatus(const std::string& tournamentId) override {
        try {
            auto matches = matchRepo_->ListByTournament(tournamentId, std::nullopt);

            std::map<std::string, int> totalByRound;
            std::map<std::string, int> playedByRound;

            for (const auto& m : matches) {
                totalByRound[m.round]++;
                if (m.status == "played") {
                    playedByRound[m.round]++;
                }
            }

            nlohmann::json status;
            status["totalMatches"] = matches.size();
            status["rounds"] = nlohmann::json::object();

            for (const auto& [round, total] : totalByRound) {
                int played = playedByRound[round];
                status["rounds"][round] = {
                    {"total", total},
                    {"played", played},
                    {"pending", total - played},
                    {"complete", played == total}
                };
            }

            // Determinar fase actual
            if (totalByRound["group_stage"] > 0 &&
                playedByRound["group_stage"] < totalByRound["group_stage"]) {
                status["currentPhase"] = "group_stage";
            } else if (totalByRound["round_of_16"] > 0 &&
                       playedByRound["round_of_16"] < totalByRound["round_of_16"]) {
                status["currentPhase"] = "round_of_16";
            } else if (totalByRound["quarter_finals"] > 0 &&
                       playedByRound["quarter_finals"] < totalByRound["quarter_finals"]) {
                status["currentPhase"] = "quarter_finals";
            } else if (totalByRound["semi_finals"] > 0 &&
                       playedByRound["semi_finals"] < totalByRound["semi_finals"]) {
                status["currentPhase"] = "semi_finals";
            } else if (totalByRound["final"] > 0 &&
                       playedByRound["final"] < totalByRound["final"]) {
                status["currentPhase"] = "final";
            } else {
                status["currentPhase"] = "completed";
            }

            return status;
        } catch (const std::exception& e) {
            return std::unexpected(std::string("Error: ") + e.what());
        }
    }

    std::expected<void, std::string>
    GenerateKnockoutPhase(const std::string& tournamentId) override {
        std::cout << "[MatchDelegate] Generating Round of 16" << std::endl;

        // Verificar que fase de grupos esté completa
        auto status = GetTournamentStatus(tournamentId);
        if (!status.has_value()) {
            return std::unexpected("Failed to get tournament status");
        }

        if (!(*status)["rounds"]["group_stage"]["complete"].get<bool>()) {
            return std::unexpected("Group stage is not complete yet");
        }

        // Obtener clasificados ordenados (mejor a peor)
        auto qualified = getRankedQualifiedTeams(tournamentId);
        if (!qualified.has_value()) {
            return std::unexpected(qualified.error());
        }

        if (qualified->size() != 16) {
            return std::unexpected("Expected 16 qualified teams, got " +
                                  std::to_string(qualified->size()));
        }

        // Generar octavos: 1 vs 16, 2 vs 15, 3 vs 14, etc.
        std::vector<domain::Match> matches;
        for (size_t i = 0; i < 8; ++i) {
            domain::Match match;
            match.tournamentId = tournamentId;
            match.homeTeamId = (*qualified)[i].teamId;      // Mejor rankeado
            match.awayTeamId = (*qualified)[15 - i].teamId; // Peor rankeado
            match.round = "round_of_16";
            match.status = "pending";
            matches.push_back(match);

            std::cout << "  Match " << (i+1) << ": "
                      << (*qualified)[i].teamName << " (rank " << (i+1) << ")"
                      << " vs "
                      << (*qualified)[15-i].teamName << " (rank " << (16-i) << ")"
                      << std::endl;
        }

        return saveMatches(matches);
    }

    std::expected<void, std::string>
    AdvanceKnockoutPhase(const std::string& tournamentId) override {
        auto status = GetTournamentStatus(tournamentId);
        if (!status.has_value()) {
            return std::unexpected("Failed to get status");
        }

        std::string currentPhase = (*status)["currentPhase"].get<std::string>();

        if (currentPhase == "group_stage") {
            return GenerateKnockoutPhase(tournamentId);
        } else if (currentPhase == "round_of_16") {
            return generateQuarterFinals(tournamentId);
        } else if (currentPhase == "quarter_finals") {
            return generateSemiFinals(tournamentId);
        } else if (currentPhase == "semi_finals") {
            return generateFinal(tournamentId);
        } else if (currentPhase == "final") {
            return std::unexpected("Tournament already completed");
        }

        return std::unexpected("Cannot advance from current phase: " + currentPhase);
    }

    std::expected<std::vector<TeamStanding>, std::string>
    GetGroupStandings(const std::string& tournamentId, const std::string& groupId) override {
        return calculateGroupStandings(tournamentId, groupId);
    }

private:
    // Calcular tabla de posiciones de un grupo
    std::expected<std::vector<TeamStanding>, std::string>
    calculateGroupStandings(const std::string& tournamentId, const std::string& groupId) {
        auto allMatches = matchRepo_->ListByTournament(tournamentId, std::nullopt);

        auto groups = groupRepo_->FindByTournamentId(tournamentId);
        std::shared_ptr<domain::Group> targetGroup;
        for (const auto& g : groups) {
            if (g->Id() == groupId) {
                targetGroup = g;
                break;
            }
        }

        if (!targetGroup) {
            return std::unexpected("Group not found");
        }

        std::map<std::string, TeamStanding> standings;

        for (const auto& team : targetGroup->Teams()) {
            TeamStanding ts;
            ts.teamId = team.Id;
            ts.teamName = team.Name;
            ts.groupId = groupId;
            standings[team.Id] = ts;
        }

        for (const auto& match : allMatches) {
            if (match.round != "group_stage" || match.status != "played") continue;
            if (!match.score.has_value()) continue;

            bool homeInGroup = standings.find(match.homeTeamId) != standings.end();
            bool awayInGroup = standings.find(match.awayTeamId) != standings.end();

            if (!homeInGroup || !awayInGroup) continue;

            auto& homeTeam = standings[match.homeTeamId];
            auto& awayTeam = standings[match.awayTeamId];

            homeTeam.played++;
            awayTeam.played++;

            homeTeam.goalsFor += match.score->home;
            homeTeam.goalsAgainst += match.score->visitor;
            awayTeam.goalsFor += match.score->visitor;
            awayTeam.goalsAgainst += match.score->home;

            if (match.score->home > match.score->visitor) {
                homeTeam.won++;
                homeTeam.points += 3;
                awayTeam.lost++;
            } else if (match.score->home < match.score->visitor) {
                awayTeam.won++;
                awayTeam.points += 3;
                homeTeam.lost++;
            } else {
                homeTeam.drawn++;
                awayTeam.drawn++;
                homeTeam.points += 1;
                awayTeam.points += 1;
            }

            homeTeam.goalDifference = homeTeam.goalsFor - homeTeam.goalsAgainst;
            awayTeam.goalDifference = awayTeam.goalsFor - awayTeam.goalsAgainst;
        }

        std::vector<TeamStanding> result;
        for (const auto& [id, standing] : standings) {
            result.push_back(standing);
        }

        std::sort(result.begin(), result.end(), [](const TeamStanding& a, const TeamStanding& b) {
            if (a.points != b.points) return a.points > b.points;
            if (a.goalDifference != b.goalDifference) return a.goalDifference > b.goalDifference;
            return a.goalsFor > b.goalsFor;
        });

        return result;
    }

    // Obtener los 16 clasificados rankeados del 1 al 16
    std::expected<std::vector<TeamStanding>, std::string>
    getRankedQualifiedTeams(const std::string& tournamentId) {
        auto groups = groupRepo_->FindByTournamentId(tournamentId);

        std::vector<TeamStanding> allQualified;

        for (const auto& group : groups) {
            auto standings = calculateGroupStandings(tournamentId, group->Id());
            if (!standings.has_value() || standings->size() < 2) {
                return std::unexpected("Failed to get standings for group: " + group->Name());
            }

            // Top 2 de cada grupo
            allQualified.push_back((*standings)[0]);
            allQualified.push_back((*standings)[1]);
        }

        // Ordenar todos los clasificados: primeros lugares, luego segundos lugares
        std::sort(allQualified.begin(), allQualified.end(),
                 [](const TeamStanding& a, const TeamStanding& b) {
            if (a.points != b.points) return a.points > b.points;
            if (a.goalDifference != b.goalDifference) return a.goalDifference > b.goalDifference;
            return a.goalsFor > b.goalsFor;
        });

        std::cout << "[MatchDelegate] Qualified teams ranking:" << std::endl;
        for (size_t i = 0; i < allQualified.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << allQualified[i].teamName
                      << " (" << allQualified[i].points << " pts)" << std::endl;
        }

        return allQualified;
    }

    // Obtener ganadores de una fase
    std::expected<std::vector<std::string>, std::string>
    getWinnersFromRound(const std::string& tournamentId, const std::string& round) {
        auto matches = matchRepo_->ListByTournament(tournamentId, std::nullopt);

        std::vector<std::string> winners;

        for (const auto& match : matches) {
            if (match.round != round || match.status != "played") continue;
            if (!match.score.has_value()) continue;

            if (match.score->home > match.score->visitor) {
                winners.push_back(match.homeTeamId);
            } else if (match.score->visitor > match.score->home) {
                winners.push_back(match.awayTeamId);
            } else {
                return std::unexpected("Match ended in draw (not supported in knockout): " + match.id);
            }
        }

        return winners;
    }

    // Generar cuartos de final
    std::expected<void, std::string>
    generateQuarterFinals(const std::string& tournamentId) {
        std::cout << "[MatchDelegate] Generating Quarter Finals" << std::endl;

        auto winners = getWinnersFromRound(tournamentId, "round_of_16");
        if (!winners.has_value()) {
            return std::unexpected(winners.error());
        }

        if (winners->size() != 8) {
            return std::unexpected("Expected 8 winners from Round of 16, got " +
                                  std::to_string(winners->size()));
        }

        std::vector<domain::Match> matches;
        for (size_t i = 0; i < winners->size(); i += 2) {
            domain::Match match;
            match.tournamentId = tournamentId;
            match.homeTeamId = (*winners)[i];
            match.awayTeamId = (*winners)[i + 1];
            match.round = "quarter_finals";
            match.status = "pending";
            matches.push_back(match);
        }

        return saveMatches(matches);
    }

    // Generar semifinales
    std::expected<void, std::string>
    generateSemiFinals(const std::string& tournamentId) {
        std::cout << "[MatchDelegate] Generating Semi Finals" << std::endl;

        auto winners = getWinnersFromRound(tournamentId, "quarter_finals");
        if (!winners.has_value()) {
            return std::unexpected(winners.error());
        }

        if (winners->size() != 4) {
            return std::unexpected("Expected 4 winners from Quarter Finals");
        }

        std::vector<domain::Match> matches;
        for (size_t i = 0; i < winners->size(); i += 2) {
            domain::Match match;
            match.tournamentId = tournamentId;
            match.homeTeamId = (*winners)[i];
            match.awayTeamId = (*winners)[i + 1];
            match.round = "semi_finals";
            match.status = "pending";
            matches.push_back(match);
        }

        return saveMatches(matches);
    }

    // Generar final
    std::expected<void, std::string>
    generateFinal(const std::string& tournamentId) {
        std::cout << "[MatchDelegate] Generating Final" << std::endl;

        auto winners = getWinnersFromRound(tournamentId, "semi_finals");
        if (!winners.has_value()) {
            return std::unexpected(winners.error());
        }

        if (winners->size() != 2) {
            return std::unexpected("Expected 2 winners from Semi Finals");
        }

        domain::Match finalMatch;
        finalMatch.tournamentId = tournamentId;
        finalMatch.homeTeamId = (*winners)[0];
        finalMatch.awayTeamId = (*winners)[1];
        finalMatch.round = "final";
        finalMatch.status = "pending";

        return saveMatches({finalMatch});
    }

    // Guardar partidos en la BD
    std::expected<void, std::string>
    saveMatches(const std::vector<domain::Match>& matches) {
        std::vector<nlohmann::json> matchJsons;
        for (const auto& match : matches) {
            nlohmann::json j;
            j["tournamentId"] = match.tournamentId;
            j["homeTeamId"] = match.homeTeamId;
            j["awayTeamId"] = match.awayTeamId;
            j["round"] = match.round;
            j["status"] = match.status;
            j["score"] = nullptr;
            matchJsons.push_back(j);
        }

        return matchRepo_->CreateBulk(matchJsons);
    }

    // Verificar y avanzar fase automáticamente
    void checkAndAdvancePhase(const std::string& tournamentId, const std::string& completedRound) {
        auto status = GetTournamentStatus(tournamentId);
        if (!status.has_value()) return;

        // Solo avanzar si la fase actual está completa
        if ((*status)["rounds"][completedRound]["complete"].get<bool>()) {
            std::cout << "[MatchDelegate] Phase " << completedRound
                      << " complete! Auto-advancing..." << std::endl;

            auto result = AdvanceKnockoutPhase(tournamentId);
            if (result.has_value()) {
                std::cout << "[MatchDelegate] Next phase generated successfully!" << std::endl;
            } else {
                std::cerr << "[MatchDelegate] Failed to advance: " << result.error() << std::endl;
            }
        }
    }
};
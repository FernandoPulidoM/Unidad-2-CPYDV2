#pragma once
#include <string>
#include <optional>
#include <vector>
#include <expected>
#include "domain/Match.hpp"
#include <nlohmann/json.hpp>

struct TeamStanding {
    std::string teamId;
    std::string teamName;
    std::string groupId;  // ← AGREGAR para tracking
    int played = 0;
    int won = 0;
    int drawn = 0;
    int lost = 0;
    int goalsFor = 0;
    int goalsAgainst = 0;
    int goalDifference = 0;
    int points = 0;
};

struct IMatchDelegate {
    virtual ~IMatchDelegate() = default;

    virtual std::vector<domain::Match>
    List(const std::string& tournamentId,
         const std::optional<std::string>& filter) = 0;

    virtual std::optional<domain::Match>
    Get(const std::string& tournamentId, const std::string& matchId) = 0;

    virtual std::expected<void, std::string>
    updateScore(const std::string& tournamentId, const std::string& matchId,
                int home, int visitor) = 0;

    virtual std::expected<void, std::string>
    GenerateMatchesForTournament(const std::string& tournamentId) = 0;

    virtual std::expected<std::vector<TeamStanding>, std::string>
    GetGroupStandings(const std::string& tournamentId, const std::string& groupId) = 0;

    virtual std::expected<nlohmann::json, std::string>
    GetTournamentStatus(const std::string& tournamentId) = 0;

    virtual std::expected<void, std::string>
    GenerateKnockoutPhase(const std::string& tournamentId) = 0;

    // NUEVO: Generar siguiente fase automáticamente
    virtual std::expected<void, std::string>
    AdvanceKnockoutPhase(const std::string& tournamentId) = 0;
};
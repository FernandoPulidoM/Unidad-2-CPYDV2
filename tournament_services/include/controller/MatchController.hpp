//
// Created by fmendivil on 10/22/25.
//
#ifndef TOURNAMENTS_MATCHCONTROLLER_HPP
#define TOURNAMENTS_MATCHCONTROLLER_HPP

#include <memory>
#include <crow.h>
#include "delegate/IMatchDelegate.hpp"

class MatchController {
    std::shared_ptr<IMatchDelegate> matchDelegate;

public:
    explicit MatchController(std::shared_ptr<IMatchDelegate> delegate);

    // POST /tournaments/{id}/matches/generate
    [[nodiscard]] crow::response GenerateBracket(
        const crow::request& request,
        const std::string& tournamentId
    ) const;

    // GET /tournaments/{id}/matches
    [[nodiscard]] crow::response GetMatches(const std::string& tournamentId) const;

    // GET /tournaments/{id}/matches/phase/{phase}
    [[nodiscard]] crow::response GetMatchesByPhase(
        const std::string& tournamentId,
        const std::string& phase
    ) const;

    // GET /matches/{id}
    [[nodiscard]] crow::response GetMatch(const std::string& matchId) const;

    // PUT /matches/{id}/score
    [[nodiscard]] crow::response UpdateScore(
        const crow::request& request,
        const std::string& matchId
    ) const;

    // POST /matches/{id}/complete
    [[nodiscard]] crow::response CompleteMatch(const std::string& matchId) const;

    // GET /tournaments/{id}/status
    [[nodiscard]] crow::response GetTournamentStatus(const std::string& tournamentId) const;
};

#endif
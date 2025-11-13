#pragma once
#include <string>
#include <optional>
#include <vector>
#include <expected>
#include "domain/Match.hpp"

// Interfaz sin namespace (coincide con como la usa MatchController)
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
};

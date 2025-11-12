#pragma once
#include <string>
#include <optional>
#include <vector>
#include <expected>

#include "domain/Match.hpp"

struct IMatchDelegate {
    virtual ~IMatchDelegate() = default;

    // Lista los matches de un torneo; filtro: "played" | "pending" | null
    virtual std::vector<domain::Match>
    List(const std::string& tournamentId,
         const std::optional<std::string>& filter) = 0;

    // Obtiene un match por id dentro de un torneo
    virtual std::optional<domain::Match>
    Get(const std::string& tournamentId, const std::string& matchId) = 0;

    // Actualiza el score (este metodo es camelCase)
    virtual std::expected<void, std::string>
    updateScore(const std::string& tournamentId,
                const std::string& matchId,
                int home, int visitor) = 0;
};

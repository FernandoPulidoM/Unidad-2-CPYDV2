#ifndef SERVICE_IMATCH_DELEGATE_HPP
#define SERVICE_IMATCH_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <expected>

#include "domain/Match.hpp"
#include "domain/Team.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;
    
    // Generar bracket de eliminación simple
    virtual std::expected<std::vector<std::string>, std::string> 
        GenerateSingleEliminationBracket(
            const std::string_view& tournamentId,
            const std::vector<domain::Team>& teams
        ) = 0;
    
    // Obtener partidos por torneo
    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatchesByTournament(const std::string_view& tournamentId) = 0;
    
    // Obtener partidos por fase
    virtual std::expected<std::vector<std::shared_ptr<domain::Match>>, std::string>
        GetMatchesByPhase(
            const std::string_view& tournamentId,
            domain::TournamentPhase phase
        ) = 0;
    
    // Obtener partido específico
    virtual std::expected<std::shared_ptr<domain::Match>, std::string>
        GetMatch(const std::string_view& matchId) = 0;
    
    // Actualizar marcador
    virtual std::expected<void, std::string>
        UpdateScore(
            const std::string_view& matchId,
            int scoreTeam1,
            int scoreTeam2
        ) = 0;
    
    // Completar partido (determina ganador y avanza al siguiente)
    virtual std::expected<void, std::string>
        CompleteMatch(const std::string_view& matchId) = 0;
    
    // Obtener estado del torneo
    virtual std::expected<std::string, std::string>
        GetTournamentStatus(const std::string_view& tournamentId) = 0;
};

#endif
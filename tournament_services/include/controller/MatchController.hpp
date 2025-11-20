#pragma once

#include <memory>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <crow.h>
#include "../delegate/IMatchDelegate.hpp"

namespace services {

    class MatchController {
    public:
        // Solo declaracion, implementacion en el .cpp para evitar doble definicion
        explicit MatchController(std::shared_ptr<IMatchDelegate> delegate);

        std::string GetMatches(const crow::request& req,
                               const std::string& tournamentId);

        std::string GetMatch(const crow::request& req,
                             const std::string& tournamentId,
                             const std::string& matchId);

        int PatchScore(const crow::request& req,
                       const std::string& tournamentId,
                       const std::string& matchId);

        int GenerateMatches(const crow::request& req,
                            const std::string& tournamentId);

        // Generar fase de eliminacion
        int GenerateKnockoutPhase(const crow::request& req,
                                  const std::string& tournamentId);

        // Avanzar fase de eliminacion
        int AdvanceKnockoutPhase(const crow::request& req,
                                 const std::string& tournamentId);

        // Estado general del torneo (matches)
        std::string GetTournamentStatus(const crow::request& req,
                                        const std::string& tournamentId);

    private:
        std::shared_ptr<IMatchDelegate> delegate_;
    };

} // namespace services

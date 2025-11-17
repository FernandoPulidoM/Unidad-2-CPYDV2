#pragma once
#include <memory>
#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include <crow.h>
#include "delegate/IMatchDelegate.hpp"

namespace services {

    class MatchController {
    public:
        explicit MatchController(std::shared_ptr<IMatchDelegate> delegate)
            : delegate_(std::move(delegate)) {}

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

        // Agregar a la clase:
        int GenerateKnockoutPhase(const crow::request& req, const std::string& tournamentId);

        // MatchController.hpp - Agregar:
        int AdvanceKnockoutPhase(const crow::request& req, const std::string& tournamentId);

        // NUEVO: Agregar declaración
        std::string GetTournamentStatus(const crow::request& req,
                                        const std::string& tournamentId);

    private:
        std::shared_ptr<IMatchDelegate> delegate_;
    };

} // namespace services
#pragma once
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include <crow.h>

#include "delegate/IMatchDelegate.hpp"
#include "configuration/RouteDefinition.hpp" // REGISTER_ROUTE

namespace services {

    class MatchController {
    public:
        explicit MatchController(std::shared_ptr<IMatchDelegate> delegate)
          : delegate_(std::move(delegate)) {}

        // GET /tournaments/<string>/matches?showMatches=played|pending
        // NOTA: el macro de Crow pasa req + params de ruta; los query params se leen desde req.
        std::string GetMatches(const crow::request& req,
                               const std::string& tournamentId);

        // GET /tournaments/<string>/matches/<string>
        std::string GetMatch(const crow::request& req,
                             const std::string& tournamentId,
                             const std::string& matchId);

        // PATCH /tournaments/<string>/matches/<string>
        // body JSON: { "score": { "home": int, "visitor": int } }
        int PatchScore(const crow::request& req,
                       const std::string& tournamentId,
                       const std::string& matchId);

    private:
        std::shared_ptr<IMatchDelegate> delegate_;
    };

    // Rutas (dentro del namespace y sin calificador de namespace en el tipo)
    REGISTER_ROUTE(MatchController, GetMatches,
                   "/tournaments/<string>/matches", "GET"_method)
    REGISTER_ROUTE(MatchController, GetMatch,
                   "/tournaments/<string>/matches/<string>", "GET"_method)
    REGISTER_ROUTE(MatchController, PatchScore,
                   "/tournaments/<string>/matches/<string>", "PATCH"_method)

} // namespace services

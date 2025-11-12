#pragma once
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>
#include "delegate/IMatchDelegate.hpp"
#include "configuration/RouteDefinition.hpp" // donde esta REGISTER_ROUTE

namespace services {

    class MatchController {
    public:
        explicit MatchController(std::shared_ptr<IMatchDelegate> delegate)
          : delegate_(std::move(delegate)) {}

        // GET /tournaments/<string>/matches?showMatches=played|pending
        nlohmann::json GetMatches(const std::string& tournamentId,
                                  const std::optional<std::string>& showMatches);

        // GET /tournaments/<string>/matches/<string>
        nlohmann::json GetMatch(const std::string& tournamentId,
                                const std::string& matchId);

        // PATCH /tournaments/<string>/matches/<string>
        // body: { "score": { "home": int, "visitor": int } }
        int PatchScore(const std::string& tournamentId,
                       const std::string& matchId,
                       const nlohmann::json& body);

    private:
        std::shared_ptr<IMatchDelegate> delegate_;
    };

} // namespace services

// Rutas
REGISTER_ROUTE(services::MatchController, GetMatches,
               "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(services::MatchController, GetMatch,
               "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(services::MatchController, PatchScore,
               "/tournaments/<string>/matches/<string>", "PATCH"_method)

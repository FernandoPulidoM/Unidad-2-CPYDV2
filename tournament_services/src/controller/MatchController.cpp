//
// Created by fmendivil on 10/22/25.
//

#include "controller/MatchController.hpp"
#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate)
    : matchDelegate(std::move(delegate)) {}

crow::response MatchController::GenerateBracket(
    const crow::request& request,
    const std::string& tournamentId
) const {
    try {
        if (!nlohmann::json::accept(request.body)) {
            return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
        }

        auto body = nlohmann::json::parse(request.body);
        std::vector<domain::Team> teams = body["teams"];

        auto result = matchDelegate->GenerateSingleEliminationBracket(tournamentId, teams);

        if (result) {
            nlohmann::json response = {
                {"matchIds", *result},
                {"totalMatches", result->size()}
            };

            return crow::response{crow::CREATED, response.dump()};
        } else {
            return crow::response{crow::BAD_REQUEST, result.error()};
        }
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response MatchController::GetMatches(const std::string& tournamentId) const {
    auto result = matchDelegate->GetMatchesByTournament(tournamentId);

    if (result) {
        nlohmann::json body = *result;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }
}

crow::response MatchController::GetMatchesByPhase(
    const std::string& tournamentId,
    const std::string& phaseStr
) const {
    // Convertir string → enum
    domain::TournamentPhase phase;
    if (phaseStr == "QUARTER_FINALS") phase = domain::TournamentPhase::QUARTER_FINALS;
    else if (phaseStr == "SEMI_FINALS") phase = domain::TournamentPhase::SEMI_FINALS;
    else if (phaseStr == "FINAL") phase = domain::TournamentPhase::FINAL;
    else if (phaseStr == "THIRD_PLACE") phase = domain::TournamentPhase::THIRD_PLACE;
    else {
        return crow::response{crow::BAD_REQUEST, "Invalid phase"};
    }

    auto result = matchDelegate->GetMatchesByPhase(tournamentId, phase);

    if (result) {
        nlohmann::json body = *result;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{crow::NOT_FOUND, result.error()};
    }
}

crow::response MatchController::GetMatch(const std::string& matchId) const {
    auto result = matchDelegate->GetMatch(matchId);

    if (result) {
        nlohmann::json body = *result.value();
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{crow::NOT_FOUND, result.error()};
    }
}

crow::response MatchController::UpdateScore(
    const crow::request& request,
    const std::string& matchId
) const {
    try {
        if (!nlohmann::json::accept(request.body)) {
            return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
        }

        auto body = nlohmann::json::parse(request.body);
        int scoreTeam1 = body["scoreTeam1"];
        int scoreTeam2 = body["scoreTeam2"];

        auto result = matchDelegate->UpdateScore(matchId, scoreTeam1, scoreTeam2);

        if (result) {
            return crow::response{crow::NO_CONTENT};
        } else {
            return crow::response{crow::BAD_REQUEST, result.error()};
        }
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response MatchController::CompleteMatch(const std::string& matchId) const {
    auto result = matchDelegate->CompleteMatch(matchId);

    if (result) {
        return crow::response{crow::NO_CONTENT};
    } else {
        return crow::response{crow::BAD_REQUEST, result.error()};
    }
}

crow::response MatchController::GetTournamentStatus(const std::string& tournamentId) const {
    auto result = matchDelegate->GetTournamentStatus(tournamentId);

    if (result) {
        nlohmann::json body = {
            {"tournamentId", tournamentId},
            {"status", *result}
        };

        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } else {
        return crow::response{crow::INTERNAL_SERVER_ERROR, result.error()};
    }
}

// Registrar rutas
REGISTER_ROUTE(MatchController, GenerateBracket, "/tournaments/<string>/matches/generate", "POST"_method)
REGISTER_ROUTE(MatchController, GetMatches, "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(MatchController, GetMatchesByPhase, "/tournaments/<string>/matches/phase/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, GetMatch, "/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, UpdateScore, "/matches/<string>/score", "PUT"_method)
REGISTER_ROUTE(MatchController, CompleteMatch, "/matches/<string>/complete", "POST"_method)
REGISTER_ROUTE(MatchController, GetTournamentStatus, "/tournaments/<string>/status", "GET"_method)
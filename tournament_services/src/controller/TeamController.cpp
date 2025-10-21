//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"


TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
        nlohmann::json body = team;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    return crow::response{crow::NOT_FOUND, "team not found"};
}

crow::response TeamController::getAllTeams() const {

    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{200, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        if(!nlohmann::json::accept(request.body)) {
            return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
        }

        auto requestBody = nlohmann::json::parse(request.body);
        domain::Team team = requestBody;

        // ← IMPORTANTE: Asignar el id de la URL
        team.Id = teamId;

        teamDelegate->UpdateTeam(teamId, team);

        crow::response response;
        response.code = crow::OK;
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } catch (const std::runtime_error& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

    // Validar formato JSON
    if (!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }

    auto body = nlohmann::json::parse(request.body);
    if (!body.contains("name")) {
        response.code = crow::BAD_REQUEST;
        return response;
    }

    domain::Team team = body;

    // Validar duplicados (nombre o id)
    auto existing = teamDelegate->GetAllTeams();
    for (auto& t : existing) {
        if (t->Name == team.Name) {
            response.code = crow::CONFLICT;
            response.body = R"({"error":"Team already exists"})";
            return response;
        }
    }

    try {
        auto newId = teamDelegate->SaveTeam(team);
        nlohmann::json respJson = {{"id", newId}, {"name", team.Name}};
        response.code = crow::CREATED;
        response.add_header("location", newId.data());
        response.add_header("content-type", "application/json");
        response.body = respJson.dump();
        return response;
    } catch (const std::exception& e) {
        response.code = crow::INTERNAL_SERVER_ERROR;
        response.body = std::string("error creating team: ") + e.what();
        return response;
    }
}

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, ID_VALUE)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        teamDelegate->DeleteTeam(teamId);
        return crow::response{crow::NO_CONTENT};
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        if (msg.find("not found") != std::string::npos)
            return crow::response{crow::NOT_FOUND, e.what()};
        else
            return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }

}



REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)

// Agregar al final con los otros REGISTER_ROUTE:
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
// Al final, después de los otros REGISTER_ROUTE:
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PUT"_method)
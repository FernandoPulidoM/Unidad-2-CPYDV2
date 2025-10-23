//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"

#include <regex>
#include <string>
#include <string_view>

static bool is_conflict_message(const std::string& msg) {
    // Heurística simple para mapear errores de constraint/duplicado a 409
    std::string m = msg;
    for (auto& c : m) c = static_cast<char>(::tolower(c));
    return m.find("constraint") != std::string::npos
        || m.find("duplicate")  != std::string::npos
        || m.find("unique")     != std::string::npos
        || m.find("already exists") != std::string::npos;
}

TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate)
    : teamDelegate(teamDelegate) {}

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
    crow::response response{crow::OK, body.dump()};
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

        // Los tests esperan 204 en actualización exitosa
        return crow::response{crow::NO_CONTENT};
    } catch (const std::runtime_error& e) {
        return crow::response{crow::NOT_FOUND, e.what()};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    // Validar formato JSON
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST};
    }

    auto body = nlohmann::json::parse(request.body);
    if (!body.contains("name") || !body["name"].is_string()) {
        return crow::response{crow::BAD_REQUEST};
    }

    domain::Team team = body;

    // Validar duplicados (por nombre)
    auto existing = teamDelegate->GetAllTeams();
    for (auto& t : existing) {
        if (t->Name == team.Name) {
            crow::response conflict{crow::CONFLICT};
            conflict.body = R"({"error":"Team already exists"})";
            conflict.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
            return conflict;
        }
    }

    try {
        auto newId = teamDelegate->SaveTeam(team);

        nlohmann::json respJson = {{"id", newId}, {"name", team.Name}};
        crow::response response{crow::CREATED, respJson.dump()};
        response.add_header("location", std::string{newId});
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    } catch (const std::runtime_error& e) {
        // Mapear constraint/duplicado a 409; otros errores a 500
        if (is_conflict_message(e.what())) {
            return crow::response{crow::CONFLICT, std::string{"error creating team: "} + e.what()};
        }
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string{"error creating team: "} + e.what()};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, std::string{"error creating team: "} + e.what()};
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
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PUT"_method)

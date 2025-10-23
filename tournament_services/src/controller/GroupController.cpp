#include "controller/GroupController.hpp"
#include "configuration/RouteDefinition.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

GroupController::GroupController(const std::shared_ptr<IGroupDelegate>& delegate)
    : groupDelegate(std::move(delegate)) {}

GroupController::~GroupController() {}

crow::response GroupController::GetGroups(const std::string& tournamentId) {
    if (auto groups = this->groupDelegate->GetGroups(tournamentId)) {
        const nlohmann::json body = *groups;
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    // Si falla, asumimos error del dominio (no se pidió diferenciar más en tests)
    return crow::response{crow::INTERNAL_SERVER_ERROR};
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) {
    auto r = this->groupDelegate->GetGroup(tournamentId, groupId);
    if (r.has_value()) {
        const nlohmann::json body = r.value();
        crow::response response{crow::OK, body.dump()};
        response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return response;
    }
    // Los tests esperan 404 cuando el delegate regresa unexpected(...)
    return crow::response{crow::NOT_FOUND, r.error()};
}

crow::response GroupController::CreateGroup(const crow::request& request, const std::string& tournamentId) {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Group group = requestBody;

    auto groupId = groupDelegate->CreateGroup(tournamentId, group);
    crow::response response;
    if (groupId) {
        response.add_header("location", *groupId);
        response.code = crow::CREATED;
    } else {
        // Tus tests mapean fallas de negocio a 422
        response.code = 422;
        response.body = groupId.error();
    }
    return response;
}

crow::response GroupController::UpdateGroup(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    if(!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Group group = requestBody;

        // Asignar IDs del path
        group.Id() = groupId;
        group.TournamentId() = tournamentId;

        auto result = groupDelegate->UpdateGroup(tournamentId, group);
        if (result) {
            // Los tests esperan 204 en éxito
            return crow::response{crow::NO_CONTENT};
        } else {
            // Y 404 cuando el delegate indica "Group doesn't exist"
            return crow::response{crow::NOT_FOUND, result.error()};
        }
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response GroupController::DeleteGroup(const std::string& tournamentId, const std::string& groupId) {
    try {
        auto result = groupDelegate->RemoveGroup(tournamentId, groupId);
        if (result) {
            return crow::response{crow::NO_CONTENT};
        } else {
            return crow::response{crow::NOT_FOUND, result.error()};
        }
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, e.what()};
    }
}

crow::response GroupController::UpdateTeams(const crow::request& request,
                                            const std::string& tournamentId,
                                            const std::string& groupId) {
    if (!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON"};
    }

    const std::vector<domain::Team> teams = nlohmann::json::parse(request.body);
    const auto result = groupDelegate->UpdateTeams(tournamentId, groupId, teams);
    if (result) {
        return crow::response{crow::NO_CONTENT};
    }
    // Tus tests esperan 422 para errores de negocio aquí
    return crow::response{422, result.error()};
}

REGISTER_ROUTE(GroupController, GetGroups, "/tournaments/<string>/groups", "GET"_method)
REGISTER_ROUTE(GroupController, GetGroup, "/tournaments/<string>/groups/<string>", "GET"_method)
REGISTER_ROUTE(GroupController, CreateGroup, "/tournaments/<string>/groups", "POST"_method)
REGISTER_ROUTE(GroupController, UpdateGroup, "/tournaments/<string>/groups/<string>", "PUT"_method)
REGISTER_ROUTE(GroupController, DeleteGroup, "/tournaments/<string>/groups/<string>", "DELETE"_method)
REGISTER_ROUTE(GroupController, UpdateTeams, "/tournaments/<string>/groups/<string>/teams", "PATCH"_method)

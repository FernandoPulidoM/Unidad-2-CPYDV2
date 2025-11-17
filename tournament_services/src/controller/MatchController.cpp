//
// Created by HiramZ04 on 11/12/25.
//
#include "controller/MatchController.hpp"
#include "domain/Match.hpp"
#include "configuration/RouteDefinition.hpp"
#include <stdexcept>

using nlohmann::json;

namespace {

json matchToJson(const domain::Match& m) {
    json j;
    j["id"]           = m.id;
    j["tournamentId"] = m.tournamentId;
    j["homeTeamId"]   = m.homeTeamId;
    j["awayTeamId"]   = m.awayTeamId;
    j["round"]        = m.round;
    j["status"]       = m.status;
    if (m.score.has_value()) {
        j["score"] = { {"home", m.score->home}, {"visitor", m.score->visitor} };
    } else {
        j["score"] = nullptr;
    }
    return j;
}

} // anon

namespace services {

std::string MatchController::GetMatches(const crow::request& req,
                                        const std::string& tournamentId) {
    std::optional<std::string> show;
    if (const char* v = req.url_params.get("showMatches")) {
        show = std::string(v);
    }

    auto list = delegate_->List(tournamentId, show);

    json out = json::array();
    for (const auto& m : list) out.push_back(matchToJson(m));
    return out.dump();
}

std::string MatchController::GetMatch(const crow::request& /*req*/,
                                      const std::string& tournamentId,
                                      const std::string& matchId) {
    auto m = delegate_->Get(tournamentId, matchId);
    if (!m) throw std::runtime_error("404");
    return matchToJson(*m).dump();
}

    int MatchController::PatchScore(const crow::request& req,
                                    const std::string& tournamentId,
                                    const std::string& matchId) {
    // Log para debugging
    std::cout << "PATCH Score - Tournament: " << tournamentId
              << ", Match: " << matchId << std::endl;

    json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.contains("score") || !body["score"].is_object()) {
        std::cerr << "Invalid JSON body" << std::endl;
        throw std::runtime_error("422");
    }

    const auto& score = body["score"];
    if (!score.contains("home") || !score.contains("visitor")) {
        std::cerr << "Missing home or visitor score" << std::endl;
        throw std::runtime_error("422");
    }

    int home    = score["home"].get<int>();
    int visitor = score["visitor"].get<int>();

    std::cout << "Updating score: " << home << "-" << visitor << std::endl;

    auto result = delegate_->updateScore(tournamentId, matchId, home, visitor);
    if (!result.has_value()) {
        std::cerr << "Update failed: " << result.error() << std::endl;
        throw std::runtime_error("500: " + result.error());
    }

    std::cout << "Score updated successfully" << std::endl;
    return 204;
}

    // MatchController.cpp
    std::string MatchController::GetTournamentStatus(const crow::request&,
                                                     const std::string& tid) {
    auto status = delegate_->GetTournamentStatus(tid);
    if (!status.has_value()) {
        throw std::runtime_error("500");
    }
    return status->dump();
}

    int MatchController::GenerateMatches(const crow::request& /*req*/,
                                     const std::string& tournamentId) {
    auto result = delegate_->GenerateMatchesForTournament(tournamentId);
    if (!result.has_value()) {
        throw std::runtime_error("500: " + result.error());
    }
    return 201; // Created
}
    // Agregar al final antes de los REGISTER_ROUTE:

    int MatchController::GenerateKnockoutPhase(const crow::request& /*req*/,
                                               const std::string& tournamentId) {
    auto result = delegate_->GenerateKnockoutPhase(tournamentId);
    if (!result.has_value()) {
        throw std::runtime_error("500: " + result.error());
    }
    return 201; // Created
}


    // MatchController.cpp - Implementar:
    int MatchController::AdvanceKnockoutPhase(const crow::request& /*req*/,
                                              const std::string& tournamentId) {
    auto result = delegate_->AdvanceKnockoutPhase(tournamentId);
    if (!result.has_value()) {
        throw std::runtime_error("500: " + result.error());
    }
    return 201;
}

} // namespace services



// Rutas: definirlas en UN SOLO TU (este .cpp)
namespace services {
REGISTER_ROUTE(MatchController, GetMatches,
               "/tournaments/<string>/matches", "GET"_method)
REGISTER_ROUTE(MatchController, GetMatch,
               "/tournaments/<string>/matches/<string>", "GET"_method)
REGISTER_ROUTE(MatchController, PatchScore,
               "/tournaments/<string>/matches/<string>", "PATCH"_method)

    REGISTER_ROUTE(MatchController, GenerateMatches,
               "/tournaments/<string>/matches/generate", "POST"_method)

    // Registrar ruta
 REGISTER_ROUTE(MatchController, GetTournamentStatus,
                "/tournaments/<string>/status", "GET"_method)


    // Y registrar la ruta:
    REGISTER_ROUTE(MatchController, GenerateKnockoutPhase,
                   "/tournaments/<string>/matches/generate-knockout", "POST"_method)

    // Registrar ruta:
REGISTER_ROUTE(MatchController, AdvanceKnockoutPhase,
               "/tournaments/<string>/matches/advance", "POST"_method)
} // namespace services

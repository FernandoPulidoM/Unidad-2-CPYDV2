//
// Created by HiramZ04 on 11/12/25.
//
#include "controller/MatchController.hpp"
#include "domain/Match.hpp"
#include <stdexcept>

using nlohmann::json;

namespace {

// Convierte domain::Match a JSON sin tocar el dominio.
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
    // Leer ?showMatches=played|pending del querystring
    std::optional<std::string> show;
    if (const char* v = req.url_params.get("showMatches")) {
        show = std::string(v);
    }

    // La interfaz expone List(...) en PascalCase
    auto list = delegate_->List(tournamentId, show);

    json out = json::array();
    for (const auto& m : list) out.push_back(matchToJson(m));
    return out.dump();
}

std::string MatchController::GetMatch(const crow::request& /*req*/,
                                      const std::string& tournamentId,
                                      const std::string& matchId) {
    auto m = delegate_->Get(tournamentId, matchId);
    if (!m) {
        // Tu capa de rutas traduce std::runtime_error("404") -> HTTP 404
        throw std::runtime_error("404");
    }
    return matchToJson(*m).dump();
}

int MatchController::PatchScore(const crow::request& req,
                                const std::string& tournamentId,
                                const std::string& matchId) {
    // Parsear body
    json body = json::parse(req.body, /*callback*/nullptr, /*allow_exceptions*/false);
    if (body.is_discarded() || !body.contains("score") || !body["score"].is_object())
        throw std::runtime_error("422");

    const auto& score = body["score"];
    if (!score.contains("home") || !score.contains("visitor"))
        throw std::runtime_error("422");

    int home    = score["home"].get<int>();
    int visitor = score["visitor"].get<int>();

    // Este metodo es camelCase en el delegate
    auto result = delegate_->updateScore(tournamentId, matchId, home, visitor);
    if (!result.has_value()) {
        throw std::runtime_error("500");
    }
    return 204; // No Content
}

} // namespace services

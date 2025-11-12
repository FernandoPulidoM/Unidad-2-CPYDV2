//
// Created by HiramZ04 on 11/12/25.
//
#include "controller/MatchController.hpp"
#include <stdexcept>

using nlohmann::json;
namespace services {

    nlohmann::json MatchController::GetMatches(const std::string& tournamentId,
                                               const std::optional<std::string>& showMatches) {
        auto list = delegate_->List(tournamentId, showMatches);
        json out = json::array();
        for (const auto& m : list) out.push_back(m.toJson()); // asume domain::Match::toJson()
        return out; // 200
    }

    nlohmann::json MatchController::GetMatch(const std::string& tournamentId,
                                             const std::string& matchId) {
        auto m = delegate_->Get(tournamentId, matchId);
        if (!m) throw std::runtime_error("404"); // tu framework ya traduce a HTTP 404
        return m->toJson(); // 200
    }

    int MatchController::PatchScore(const std::string& tournamentId,
                                    const std::string& matchId,
                                    const json& body) {
        if (!body.contains("score") || !body["score"].is_object())
            throw std::runtime_error("422");

        auto score = body["score"];
        if (!score.contains("home") || !score.contains("visitor"))
            throw std::runtime_error("422");

        int home = score["home"].get<int>();
        int visitor = score["visitor"].get<int>();

        auto result = delegate_->UpdateScore(tournamentId, matchId, home, visitor);
        if (!result.has_value()) {
            const auto code = result.error().code; // 404|422|500 segun validacion
            throw std::runtime_error(std::to_string(code));
        }
        return 204;
    }

} // namespace services

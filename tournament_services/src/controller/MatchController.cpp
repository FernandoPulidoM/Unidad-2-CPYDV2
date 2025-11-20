#include "controller/MatchController.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace services {

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate)
    : delegate_(std::move(delegate)) {}


// ---------------------------- GET MATCHES -----------------------------
std::string MatchController::GetMatches(
        const crow::request& req,
        const std::string& tournamentId)
{
    std::optional<std::string> filter = std::nullopt;

    // 1) Intentar leer de url_params (modo normal en Crow)
    const char* rawShowMatches = req.url_params.get("showMatches");
    const char* rawFilter      = req.url_params.get("filter");
    const char* rawStatus      = req.url_params.get("status");

    if (rawShowMatches != nullptr) {
        filter = std::string(rawShowMatches);      // "played" o "pending"
    } else if (rawFilter != nullptr) {
        filter = std::string(rawFilter);
    } else if (rawStatus != nullptr) {
        filter = std::string(rawStatus);
    }

    // 2) Si en tests no llenaron url_params, hacer fallback a parsear req.url
    if (!filter.has_value()) {
        std::string url = req.url;  // ej: "/tournaments/t1/matches?filter=pending"
        auto pos = url.find('?');
        if (pos != std::string::npos) {
            std::string qs = url.substr(pos + 1);  // "filter=pending&x=y"
            std::stringstream ss(qs);
            std::string pair;

            while (std::getline(ss, pair, '&')) {
                auto eq = pair.find('=');
                if (eq == std::string::npos) continue;

                std::string key = pair.substr(0, eq);
                std::string val = pair.substr(eq + 1);

                if (key == "showMatches" || key == "filter" || key == "status") {
                    filter = val;  // ej. "pending"
                    break;
                }
            }
        }
    }

    auto list = delegate_->List(tournamentId, filter);

    nlohmann::json j = nlohmann::json::array();
    for (auto& m : list) {
        nlohmann::json x;
        x["id"]           = m.id;
        x["tournamentId"] = m.tournamentId;
        x["homeTeamId"]   = m.homeTeamId;
        x["awayTeamId"]   = m.awayTeamId;
        x["round"]        = m.round;
        x["status"]       = m.status;

        if (m.score.has_value()) {
            x["score"] = {
                {"home",    m.score->home},
                {"visitor", m.score->visitor}
            };
        } else {
            x["score"] = nullptr;
        }
        j.push_back(x);
    }
    return j.dump();
}


// ---------------------------- GET MATCH -----------------------------
std::string MatchController::GetMatch(
        const crow::request&,
        const std::string& tournamentId,
        const std::string& matchId)
{
    auto m = delegate_->Get(tournamentId, matchId);
    if (!m.has_value()) {
        throw std::runtime_error("Match not found");
    }

    nlohmann::json x;
    x["id"]           = m->id;
    x["tournamentId"] = m->tournamentId;
    x["homeTeamId"]   = m->homeTeamId;
    x["awayTeamId"]   = m->awayTeamId;
    x["round"]        = m->round;
    x["status"]       = m->status;

    if (m->score.has_value()) {
        x["score"] = {
            {"home",    m->score->home},
            {"visitor", m->score->visitor}
        };
    } else {
        x["score"] = nullptr;
    }

    return x.dump();
}


// ---------------------------- PATCH SCORE -----------------------------
int MatchController::PatchScore(
        const crow::request& req,
        const std::string& tournamentId,
        const std::string& matchId)
{
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded())
        throw std::runtime_error("Invalid JSON");

    if (!body.contains("score"))
        throw std::runtime_error("Missing score field");

    int home    = body["score"].value("home", -999);
    int visitor = body["score"].value("visitor", -999);

    auto r = delegate_->updateScore(tournamentId, matchId, home, visitor);

    if (!r.has_value())
        throw std::runtime_error("Score update failed");

    return 204;
}


// ---------------------------- GENERATE MATCHES -----------------------------
int MatchController::GenerateMatches(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateMatchesForTournament(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("Generate failed");

    return 201;
}


// ---------------------------- GENERATE KNOCKOUT -----------------------------
int MatchController::GenerateKnockoutPhase(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateKnockoutPhase(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("KO generation failed");

    return 201;
}


// ---------------------------- ADVANCE PHASE -----------------------------
int MatchController::AdvanceKnockoutPhase(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->AdvanceKnockoutPhase(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("Advance failed");

    return 201;
}


// ---------------------------- STATUS -----------------------------
std::string MatchController::GetTournamentStatus(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->GetTournamentStatus(tournamentId);
    if (!r.has_value())
        return "{}";
    return r->dump();
}

} // namespace services

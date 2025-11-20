#include "controller/MatchController.hpp"
#include <nlohmann/json.hpp>

namespace services {

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate)
    : delegate_(std::move(delegate)) {}


// ---------------------------- GET MATCHES -----------------------------
std::string MatchController::GetMatches(
        const crow::request& req,
        const std::string& tournamentId)
{
    std::optional<std::string> filter = std::nullopt;

    // Los tests usan ?filter=pending
    const char* raw = req.url_params.get("filter");
    if (raw != nullptr) {
        filter = std::string(raw);
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

    return 204;  // lo que piden los tests
}


// ---------------------------- GENERATE MATCHES -----------------------------
int MatchController::GenerateMatches(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateMatchesForTournament(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("Generate failed");

    return 201;  // lo que piden los tests
}


// ---------------------------- GENERATE KNOCKOUT -----------------------------
int MatchController::GenerateKnockoutPhase(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateKnockoutPhase(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("KO generation failed");

    return 201;  // tests piden esto
}


// ---------------------------- ADVANCE PHASE -----------------------------
int MatchController::AdvanceKnockoutPhase(
        const crow::request&,
        const std::string& tournamentId)
{
    auto r = delegate_->AdvanceKnockoutPhase(tournamentId);
    if (!r.has_value())
        throw std::runtime_error("Advance failed");

    return 201;  // tests piden esto
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

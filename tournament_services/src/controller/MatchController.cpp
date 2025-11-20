#include "controller/MatchController.hpp"

namespace services {

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate)
    : delegate_(std::move(delegate)) {}

// ===============================================================
// GET /tournaments/:tid/matches
// ===============================================================
std::string MatchController::GetMatches(const crow::request& req, const std::string& tournamentId)
{
    auto list = delegate_->List(tournamentId, std::nullopt);

    nlohmann::json j = nlohmann::json::array();
    for (const auto& m : list) {
        j.push_back(m.ToJson());
    }
    return j.dump();
}

// ===============================================================
// GET /tournaments/:tid/matches/:id
// ===============================================================
std::string MatchController::GetMatch(const crow::request&, const std::string& tid, const std::string& mid)
{
    auto result = delegate_->Get(tid, mid);
    if (!result)
        return nlohmann::json{{"error", "not-found"}}.dump();

    return result->ToJson().dump();
}

// ===============================================================
// PATCH /tournaments/:tid/matches/:id/score
// ===============================================================
int MatchController::PatchScore(const crow::request& req, const std::string& tid, const std::string& mid)
{
    auto body = nlohmann::json::parse(req.body);

    int home = body["score"]["home"];
    int visitor = body["score"]["visitor"];

    auto r = delegate_->updateScore(tid, mid, home, visitor);
    if (!r)
        return 400;

    return 200;
}

// ===============================================================
// POST /tournaments/:tid/matches/generate
// ===============================================================
int MatchController::GenerateMatches(const crow::request&, const std::string& tid)
{
    auto r = delegate_->GenerateMatchesForTournament(tid);
    if (!r)
        return 400;
    return 200;
}

// ===============================================================
// POST /tournaments/:tid/knockout/generate
// ===============================================================
int MatchController::GenerateKnockoutPhase(const crow::request&, const std::string& tid)
{
    auto r = delegate_->GenerateKnockoutPhase(tid);
    return r ? 200 : 400;
}

// ===============================================================
// POST /tournaments/:tid/knockout/advance
// ===============================================================
int MatchController::AdvanceKnockoutPhase(const crow::request&, const std::string& tid)
{
    auto r = delegate_->AdvanceKnockoutPhase(tid);
    return r ? 200 : 400;
}

// ===============================================================
// GET /tournaments/:tid/status
// ===============================================================
std::string MatchController::GetTournamentStatus(const crow::request&, const std::string& tid)
{
    auto r = delegate_->GetTournamentStatus(tid);
    if (!r)
        return nlohmann::json{{"error", r.error()}}.dump();

    return r->dump();
}

} // namespace services

#include "controller/MatchController.hpp"
#include <nlohmann/json.hpp>

namespace services {

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate)
    : delegate_(std::move(delegate)) {}


std::string MatchController::GetMatches(
        const crow::request& /*req*/,
        const std::string& tournamentId)
{
    auto list = delegate_->List(tournamentId, std::nullopt);

    nlohmann::json j = nlohmann::json::array();
    for (const auto& m : list)
    {
        nlohmann::json x;
        x["id"] = m.id;
        x["tournamentId"] = m.tournamentId;
        x["homeTeamId"] = m.homeTeamId;
        x["awayTeamId"] = m.awayTeamId;
        x["round"] = m.round;
        x["status"] = m.status;

        if (m.score.has_value()) {
            x["score"] = {
                {"home", m.score->home},
                {"visitor", m.score->visitor}
            };
        } else {
            x["score"] = nullptr;
        }

        j.push_back(x);
    }
    return j.dump();
}


std::string MatchController::GetMatch(
        const crow::request& /*req*/,
        const std::string& tournamentId,
        const std::string& matchId)
{
    auto m = delegate_->Get(tournamentId, matchId);
    if (!m.has_value())
        return "{}";

    nlohmann::json x;
    x["id"] = m->id;
    x["tournamentId"] = m->tournamentId;
    x["homeTeamId"] = m->homeTeamId;
    x["awayTeamId"] = m->awayTeamId;
    x["round"] = m->round;
    x["status"] = m->status;

    if (m->score.has_value()) {
        x["score"] = {
            {"home", m->score->home},
            {"visitor", m->score->visitor}
        };
    } else {
        x["score"] = nullptr;
    }

    return x.dump();
}


int MatchController::PatchScore(
        const crow::request& req,
        const std::string& tournamentId,
        const std::string& matchId)
{
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        return 400;
    }

    if (!body.contains("score")) {
        return 400;
    }

    int home = body["score"].value("home", -1);
    int visitor = body["score"].value("visitor", -1);

    auto result = delegate_->updateScore(tournamentId, matchId, home, visitor);
    if (!result.has_value()) {
        return 400;
    }

    return 200;
}


int MatchController::GenerateMatches(
        const crow::request& /*req*/,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateMatchesForTournament(tournamentId);
    return r.has_value() ? 200 : 400;
}


int MatchController::GenerateKnockoutPhase(
        const crow::request& /*req*/,
        const std::string& tournamentId)
{
    auto r = delegate_->GenerateKnockoutPhase(tournamentId);
    return r.has_value() ? 200 : 400;
}


int MatchController::AdvanceKnockoutPhase(
        const crow::request& /*req*/,
        const std::string& tournamentId)
{
    auto r = delegate_->AdvanceKnockoutPhase(tournamentId);
    return r.has_value() ? 200 : 400;
}


std::string MatchController::GetTournamentStatus(
        const crow::request& /*req*/,
        const std::string& tournamentId)
{
    auto r = delegate_->GetTournamentStatus(tournamentId);
    if (!r.has_value())
        return "{}";
    return r->dump();
}

} // namespace services

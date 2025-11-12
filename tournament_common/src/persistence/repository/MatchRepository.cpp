//
// Created by HiramZ04 on 11/12/25.
//
#include "tournament_common/include/persistence/repository/MatchRepository.hpp"
#include <pqxx/pqxx>

using namespace domain;

namespace persistence {

static Match rowToMatch(const pqxx::row& r) {
  Match m;
  m.id = r["id"].c_str();
  m.tournamentId = r["tournament_id"].c_str();
  m.homeTeamId = r["home_team_id"].c_str();
  m.awayTeamId = r["away_team_id"].c_str();
  m.round = r["round"].c_str(); // "regular" o "elimination"
  if (!r["score_home"].is_null()) m.scoreHome = r["score_home"].as<int>();
  if (!r["score_away"].is_null()) m.scoreAway = r["score_away"].as<int>();
  return m;
}

std::vector<Match>
MatchRepository::ListByTournament(const std::string& tid,
                                  const std::optional<std::string>& filter) {
  pqxx::connection c(db_->connectionString());
  pqxx::work tx(c);
  std::string q =
    "SELECT id,tournament_id,home_team_id,away_team_id,round,score_home,score_away "
    "FROM matches WHERE tournament_id = " + tx.quote(tid);
  if (filter) {
    if (*filter == "played")      q += " AND score_home IS NOT NULL AND score_away IS NOT NULL";
    else if (*filter == "pending") q += " AND (score_home IS NULL OR score_away IS NULL)";
  }
  q += " ORDER BY id";
  auto res = tx.exec(q);
  std::vector<Match> out;
  out.reserve(res.size());
  for (const auto& r : res) out.push_back(rowToMatch(r));
  tx.commit();
  return out;
}

std::optional<Match>
MatchRepository::GetById(const std::string& tid, const std::string& mid) {
  pqxx::connection c(db_->connectionString());
  pqxx::work tx(c);
  auto res = tx.exec_params(
    "SELECT id,tournament_id,home_team_id,away_team_id,round,score_home,score_away "
    "FROM matches WHERE tournament_id=$1 AND id=$2", tid, mid);
  if (res.empty()) return std::nullopt;
  return rowToMatch(res[0]);
}

std::expected<void, std::string>
MatchRepository::UpdateScore(const std::string& tid, const std::string& mid,
                             int h, int v) {
  try {
    pqxx::connection c(db_->connectionString());
    pqxx::work tx(c);
    tx.exec_params("UPDATE matches SET score_home=$1, score_away=$2 WHERE tournament_id=$3 AND id=$4",
                   h, v, tid, mid);
    tx.commit();
    return {};
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}

bool MatchRepository::ExistsPairing(const std::string& tid,
                                    const std::string& h, const std::string& a) {
  pqxx::connection c(db_->connectionString());
  pqxx::work tx(c);
  auto res = tx.exec_params(
    "SELECT 1 FROM matches WHERE tournament_id=$1 AND "
    "((home_team_id=$2 AND away_team_id=$3) OR (home_team_id=$3 AND away_team_id=$2)) LIMIT 1",
    tid, h, a);
  return !res.empty();
}

std::expected<std::string, std::string>
MatchRepository::Create(const Match& m) {
  try{
    pqxx::connection c(db_->connectionString());
    pqxx::work tx(c);
    auto res = tx.exec_params(
      "INSERT INTO matches (tournament_id,home_team_id,away_team_id,round) "
      "VALUES ($1,$2,$3,$4) RETURNING id",
      m.tournamentId, m.homeTeamId, m.awayTeamId, m.round);
    tx.commit();
    return res[0][0].c_str();
  } catch(const std::exception& e){
    return std::unexpected(e.what());
  }
}

std::expected<void, std::string>
MatchRepository::CreateBulk(const std::vector<Match>& ms) {
  try {
    pqxx::connection c(db_->connectionString());
    pqxx::work tx(c);
    for (const auto& m : ms) {
      tx.exec_params(
        "INSERT INTO matches (tournament_id,home_team_id,away_team_id,round) VALUES ($1,$2,$3,$4)",
        m.tournamentId, m.homeTeamId, m.awayTeamId, m.round);
    }
    tx.commit();
    return {};
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}

// Aqui lo dejo abstracto; conecta esto con tu capa ActiveMQ real.
void MatchRepository::PublishEvent(const std::string& address, const std::string& payload) {
  (void)address; (void)payload;
  // TODO: usa tu ConnectionManager/producer real para mandar a ActiveMQ.
}

} // namespace persistence

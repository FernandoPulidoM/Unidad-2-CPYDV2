#include "processors/ScoreRecordedEventProcessor.hpp"
#include <algorithm>
#include <unordered_map>

using nlohmann::json;
using namespace domain;

namespace consumers {

void ScoreRecordedEventProcessor::process(const json& evt) {
  const std::string tid = evt.at("tournamentId").get<std::string>();
  (void)evt;

  if (allRegularPlayed(tid)) {
    createEliminationBracketTopN(tid, 8);
  }
}

bool ScoreRecordedEventProcessor::allRegularPlayed(const std::string& tid) {
  auto all = matchRepo_->ListByTournament(tid, std::nullopt);
  for (const auto& m : all) {
    if (m.round == "regular" && (!m.scoreHome.has_value() || !m.scoreAway.has_value()))
      return false;
  }
  return true;
}

std::vector<ScoreRecordedEventProcessor::Row>
ScoreRecordedEventProcessor::computeRoundRobinTable(const std::string& tid) {
  std::unordered_map<std::string, Row> tbl;
  auto all = matchRepo_->ListByTournament(tid, std::nullopt);
  for (const auto& m : all) {
    if (m.round != "regular" || !m.scoreHome || !m.scoreAway) continue;
    auto& H = tbl[m.homeTeamId]; H.teamId = m.homeTeamId;
    auto& A = tbl[m.awayTeamId]; A.teamId = m.awayTeamId;
    H.pf += *m.scoreHome; H.pa += *m.scoreAway;
    A.pf += *m.scoreAway; A.pa += *m.scoreHome;
    if (*m.scoreHome > *m.scoreAway) ++H.wins;
    else if (*m.scoreAway > *m.scoreHome) ++A.wins;
  }
  std::vector<Row> rows;
  rows.reserve(tbl.size());
  for (auto& kv : tbl) rows.push_back(kv.second);
  std::sort(rows.begin(), rows.end(), [](const Row& x, const Row& y){
    if (x.wins != y.wins) return x.wins > y.wins;
    if (x.pf   != y.pf)   return x.pf   > y.pf;
    return x.pa < y.pa;
  });
  return rows;
}

void ScoreRecordedEventProcessor::createEliminationBracketTopN(const std::string& tid, int topN) {
  auto rows = computeRoundRobinTable(tid);
  if ((int)rows.size() < topN) return;

  std::vector<std::string> seed(topN);
  for (int i = 0; i < topN; ++i) seed[i] = rows[i].teamId;

  std::vector<nlohmann::json> docs;
  auto add = [&](const std::string& a, const std::string& b){
    nlohmann::json j = {
      {"tournamentId", tid},
      {"homeTeamId", a},
      {"awayTeamId", b},
      {"round", "elimination"}
    };
    docs.push_back(std::move(j));
  };
  add(seed[0], seed[7]);
  add(seed[3], seed[4]);
  add(seed[2], seed[5]);
  add(seed[1], seed[6]);

  (void)matchRepo_->CreateBulk(docs);
}

} // namespace consumers

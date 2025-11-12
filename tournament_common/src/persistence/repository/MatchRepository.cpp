
//
// Created by HiramZ04 on 11/12/25.
//
#include "persistence/repository/MatchRepository.hpp"
#include <pqxx/pqxx>

using nlohmann::json;
using namespace domain;

namespace {

// usa los datos de tu script de postgres
static const char* PG_CONN =
  "host=127.0.0.1 port=5432 dbname=tournament_db user=tournament_admin password=password";

} // anon namespace

namespace persistence {

Match MatchRepository::fromRow(const std::string& id, const json& j) {
  Match m = Match::fromJson(j);
  if (m.id.empty()) m.id = id;
  return m;
}

std::vector<Match>
MatchRepository::ListByTournament(const std::string& tid,
                                  const std::optional<std::string>& filter) {
  pqxx::connection c(PG_CONN);
  pqxx::work tx(c);

  std::string q =
    "SELECT id, document::text "
    "FROM matches "
    "WHERE document->>'tournamentId' = " + tx.quote(tid);

  if (filter) {
    if (*filter == "played") {
      q += " AND (document->'score'->>'home') IS NOT NULL "
           "AND (document->'score'->>'visitor') IS NOT NULL";
    } else if (*filter == "pending") {
      q += " AND ((document->'score'->>'home') IS NULL "
           "OR (document->'score'->>'visitor') IS NULL)";
    }
  }
  q += " ORDER BY created_at ASC";

  auto res = tx.exec(q);
  std::vector<Match> out;
  out.reserve(res.size());
  for (const auto& r : res) {
    const std::string id = r["id"].c_str();
    json j = json::parse(r["document"].c_str());
    out.push_back(fromRow(id, j));
  }
  tx.commit();
  return out;
}

std::optional<Match>
MatchRepository::GetById(const std::string& tid, const std::string& mid) {
  pqxx::connection c(PG_CONN);
  pqxx::work tx(c);
  auto res = tx.exec_params(
    "SELECT id, document::text "
    "FROM matches "
    "WHERE id=$1 AND document->>'tournamentId'=$2",
    mid, tid);

  if (res.empty()) return std::nullopt;
  const std::string id = res[0]["id"].c_str();
  json j = json::parse(res[0]["document"].c_str());
  tx.commit();
  return fromRow(id, j);
}

std::expected<void, std::string>
MatchRepository::UpdateScore(const std::string& tid, const std::string& mid,
                             int h, int v) {
  try {
    pqxx::connection c(PG_CONN);
    pqxx::work tx(c);

    tx.exec_params(
      "UPDATE matches "
      "SET document = jsonb_set( "
      "      jsonb_set(document, '{score,home}',   to_jsonb($1::int), true), "
      "      '{score,visitor}', to_jsonb($2::int), true), "
      "    last_update_date = CURRENT_TIMESTAMP "
      "WHERE id=$3 AND document->>'tournamentId'=$4",
      h, v, mid, tid);

    tx.commit();
    return {};
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}

bool MatchRepository::ExistsPairing(const std::string& tid,
                                    const std::string& h, const std::string& a) {
  pqxx::connection c(PG_CONN);
  pqxx::work tx(c);
  auto res = tx.exec_params(
    "SELECT 1 FROM matches "
    "WHERE document->>'tournamentId'=$1 AND "
    "((document->>'homeTeamId'=$2 AND document->>'awayTeamId'=$3) "
    " OR (document->>'homeTeamId'=$3 AND document->>'awayTeamId'=$2)) "
    "LIMIT 1",
    tid, h, a);
  return !res.empty();
}

std::expected<std::string, std::string>
MatchRepository::Create(const json& docJson) {
  try {
    pqxx::connection c(PG_CONN);
    pqxx::work tx(c);
    auto res = tx.exec_params(
      "INSERT INTO matches(document) VALUES ($1::jsonb) RETURNING id",
      docJson.dump());
    std::string id = res[0][0].c_str();
    tx.commit();
    return id;
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}

std::expected<void, std::string>
MatchRepository::CreateBulk(const std::vector<json>& docs) {
  try {
    pqxx::connection c(PG_CONN);
    pqxx::work tx(c);
    for (const auto& j : docs) {
      tx.exec_params("INSERT INTO matches(document) VALUES ($1::jsonb)", j.dump());
    }
    tx.commit();
    return {};
  } catch (const std::exception& e) {
    return std::unexpected(e.what());
  }
}

} // namespace persistence

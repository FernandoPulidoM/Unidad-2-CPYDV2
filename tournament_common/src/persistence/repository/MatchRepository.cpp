//
// Created by fmendivil on 10/22/25.
//

#include "persistence/repository/MatchRepository.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>

MatchRepository::MatchRepository(std::shared_ptr<IDbConnectionProvider> provider)
    : connectionProvider(std::move(provider)) {}

std::string MatchRepository::Create(const domain::Match& entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    nlohmann::json matchDoc = entity;

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"insert_match"}, matchDoc.dump());
    tx.commit();

    return result[0]["id"].c_str();
}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result{tx.exec("SELECT id, document FROM matches")};
    tx.commit();

    for(auto row : result) {
        nlohmann::json rowMatch = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(rowMatch);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}

std::shared_ptr<domain::Match> MatchRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_match_by_id"}, id);
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    nlohmann::json rowMatch = nlohmann::json::parse(result.at(0)["document"].c_str());
    auto match = std::make_shared<domain::Match>(rowMatch);
    match->Id() = result.at(0)["id"].c_str();

    return match;
}

std::string MatchRepository::Update(const domain::Match& entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    const nlohmann::json matchDoc = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result r = tx.exec_params(
        "UPDATE matches SET document = $1 WHERE id = $2::uuid RETURNING id;",
        matchDoc.dump(),
        entity.Id()
    );
    tx.commit();

    if (r.empty()) {
        throw std::runtime_error("Match not found");
    }

    return r[0]["id"].c_str();
}

void MatchRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result r = tx.exec_params(
        "DELETE FROM matches WHERE id = $1::uuid;",
        id
    );
    tx.commit();

    if (r.affected_rows() == 0) {
        throw std::runtime_error("Match not found");
    }
}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindByTournamentId(
    const std::string& tournamentId
) {
    std::vector<std::shared_ptr<domain::Match>> matches;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec_params(
        "SELECT id, document FROM matches WHERE document->>'tournamentId' = $1;",
        tournamentId
    );
    tx.commit();

    for(auto row : result) {
        nlohmann::json rowMatch = nlohmann::json::parse(row["document"].c_str());
        auto match = std::make_shared<domain::Match>(rowMatch);
        match->Id() = row["id"].c_str();
        matches.push_back(match);
    }

    return matches;
}
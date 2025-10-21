//
// Created by tsuny on 9/1/25.
//
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/TournamentRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"


TournamentRepository::TournamentRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {
}

std::shared_ptr<domain::Tournament> TournamentRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);


    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_tournament_by_id"}, id);
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }
    nlohmann::json rowTournament = nlohmann::json::parse(result.at(0)["document"].c_str());
    auto tournament = std::make_shared<domain::Tournament>(rowTournament);
    tournament->Id() = result.at(0)["id"].c_str();

    return tournament;
}


std::string TournamentRepository::Create (const domain::Tournament & entity) {

    const nlohmann::json tournamentDoc = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"insert_tournament"}, tournamentDoc.dump());

    tx.commit();

    return result[0]["id"].c_str();
}


std::string TournamentRepository::Update(const domain::Tournament& entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    // Usa el id del parámetro de la URL, no del JSON
    const nlohmann::json tournamentDoc = entity;

    pqxx::result r = tx.exec_params(
        "UPDATE tournaments SET document = $1 WHERE id = $2::uuid RETURNING id;",
        tournamentDoc.dump(),
        entity.Id()  // ← Usa el método Id() de la clase, no el JSON
    );

    tx.commit();

    if (r.empty()) {
        throw std::runtime_error("Tournament not found");
    }

    return r[0]["id"].c_str();
}
// ```
//
// El cambio clave: `SET document = $1` en lugar de `SET name = $1`, y `tournamentDoc.dump()` para guardar el JSON completo.
//
// Recompila y prueba:
// ```
// PUT http://localhost:8080/tournaments/043b0392-5229-4f7b-9ef6-5388c5e18e2f
//
// Body:
// {
//     "id": "043b0392-5229-4f7b-9ef6-5388c5e18e2f",
//     "name": "nombre nuevo",
//     "format": {"type": "ROUND_ROBIN", "numberOfGroups": 1, "maxTeamsPerGroup": 16}
// }
// void TournamentRepository::Delete(std::string id) {
//
// }

// Al final del archivo, agrega:
void TournamentRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result r = tx.exec_params(
        "DELETE FROM tournaments WHERE id = $1::uuid;",
        id
    );

    tx.commit();

    if (r.affected_rows() == 0) {
        throw std::runtime_error("Tournament not found");
    }
}
std::vector<std::shared_ptr<domain::Tournament>> TournamentRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Tournament>> tournaments;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result{tx.exec("select id, document from tournaments")};
    tx.commit();

    for(auto row : result){
        nlohmann::json rowTournament = nlohmann::json::parse(row["document"].c_str());
        auto tournament = std::make_shared<domain::Tournament>(rowTournament);
        tournament->Id() = row["id"].c_str();

        tournaments.push_back(tournament);
    }

    return tournaments;
}
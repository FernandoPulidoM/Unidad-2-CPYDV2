//
// Created by tsuny on 9/1/25.
//
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "persistence/repository/TeamRepository.hpp"
#include "domain/Utilities.hpp"
#include "persistence/configuration/PostgresConnection.hpp"


TeamRepository::TeamRepository(std::shared_ptr<IDbConnectionProvider> connection) : connectionProvider(std::move(connection)) {
}

std::shared_ptr<domain::Team> TeamRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);


    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_team_by_id"}, id);
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }
    nlohmann::json rowTeam = nlohmann::json::parse(result.at(0)["document"].c_str());
    auto team = std::make_shared<domain::Team>(rowTeam);
    team->Id() = result.at(0)["id"].c_str();

    return team;
}

std::string TeamRepository::Create (const domain::Team & entity) {

    const nlohmann::json teamDoc = entity;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamDoc.dump());

    tx.commit();

    return result[0]["id"].c_str();
}

std::string TeamRepository::Update(const domain::Team& entity) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    // Usa el id del parámetro de la URL, no del JSON
    const nlohmann::json teamDoc = entity;

    pqxx::result r = tx.exec_params(
        "UPDATE teams SET document = $1 WHERE id = $2::uuid RETURNING id;",
        teamDoc.dump(),
        entity.Id()  // ← Usa el método Id() de la clase, no el JSON
    );

    tx.commit();

    if (r.empty()) {
        throw std::runtime_error("Team not found");
    }

    return r[0]["id"].c_str();
}
// ```
//
// El cambio clave: `SET document = $1` en lugar de `SET name = $1`, y `teamDoc.dump()` para guardar el JSON completo.
//
// Recompila y prueba:
// ```
// PUT http://localhost:8080/teams/043b0392-5229-4f7b-9ef6-5388c5e18e2f
//
// Body:
// {
//     "id": "043b0392-5229-4f7b-9ef6-5388c5e18e2f",
//     "name": "nombre nuevo",
//     "format": {"type": "ROUND_ROBIN", "numberOfGroups": 1, "maxTeamsPerGroup": 16}
// }
// void TeamRepository::Delete(std::string id) {
//
// }

// Al final del archivo, agrega:
void TeamRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result r = tx.exec_params(
        "DELETE FROM teams WHERE id = $1::uuid;",
        id
    );

    tx.commit();

    if (r.affected_rows() == 0) {
        throw std::runtime_error("Team not found");
    }
}
std::vector<std::shared_ptr<domain::Team>> TeamRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Team>> teams;

    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result{tx.exec("select id, document from teams")};
    tx.commit();

    for(auto row : result){
        nlohmann::json rowTeam = nlohmann::json::parse(row["document"].c_str());
        auto team = std::make_shared<domain::Team>(rowTeam);
        team->Id() = row["id"].c_str();

        teams.push_back(team);
    }

    return teams;
}
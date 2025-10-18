////////////////////////////////NO usar este esta mal
//////////////////////////////////////////////////nonononononononnononononononononononononononononononononononononononononononnonononononononononononono
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
///
#include "../../include/persistence/TournamentRepository.hpp"

using namespace persistence;

TournamentRepository::TournamentRepository(const std::string& conn)
    : connectionString(conn) {}

void TournamentRepository::save(const Tournament& tournament) {
    pqxx::connection c(connectionString);
    pqxx::work txn(c);
    txn.exec_params("INSERT INTO tournaments (name) VALUES ($1);", tournament.name);
    txn.commit();
}

std::vector<Tournament> TournamentRepository::getAll() {
    pqxx::connection c(connectionString);
    pqxx::work txn(c);
    pqxx::result r = txn.exec("SELECT id, name FROM tournaments;");

    std::vector<Tournament> tournaments;
    for (const auto& row : r) {
        tournaments.push_back(Tournament{
            row["id"].as<int>(),
            row["name"].as<std::string>()
        });
    }
    return tournaments;
}

// Agregar al final del archivo, antes del cierre
void TournamentRepository::deleteById(const std::string& id) {
    pqxx::connection c(connectionString);
    pqxx::work txn(c);
    pqxx::result r = txn.exec_params(
        "DELETE FROM tournaments WHERE id = $1;",
        id
    );
    txn.commit();

    if (r.affected_rows() == 0) {
        throw std::runtime_error("Tournament not found");
    }
}
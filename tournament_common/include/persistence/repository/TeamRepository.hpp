//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
s
namespace persistence {

    class TeamRepository : public IRepository<domain::Team, std::string_view> {
        std::shared_ptr<IDbConnectionProvider> connectionProvider;

        // helper para obtener una tx como en tus otros metodos
        inline pqxx::work makeTx() {
            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
            return pqxx::work(*(connection->connection));
        }

    public:
        explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider)
            : connectionProvider(std::move(connectionProvider)) {}

        // IRepository
        std::vector<std::shared_ptr<domain::Team>> ReadAll() override {
            std::vector<std::shared_ptr<domain::Team>> teams;

            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            pqxx::work tx(*(connection->connection));
            pqxx::result result{ tx.exec("select id, document->>'name' as name from teams") };
            tx.commit();

            for (auto row : result) {
                teams.push_back(std::make_shared<domain::Team>(
                    domain::Team{ row["id"].c_str(), row["name"].c_str() }
                ));
            }
            return teams;
        }

        std::shared_ptr<domain::Team> ReadById(std::string_view id) override {
            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            pqxx::work tx(*(connection->connection));
            pqxx::result result = tx.exec(pqxx::prepped{ "select_team_by_id" }, id.data());
            tx.commit();

            auto team = std::make_shared<domain::Team>(
                nlohmann::json::parse(result[0]["document"].c_str())
            );
            team->Id = result[0]["id"].c_str();
            return team;
        }

        std::string_view Create(const domain::Team& entity) override {
            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
            nlohmann::json teamBody = entity;

            pqxx::work tx(*(connection->connection));
            pqxx::result result = tx.exec(pqxx::prepped{ "insert_team" }, teamBody.dump());
            tx.commit();

            return result[0]["id"].c_str(); // nota: tu interfaz usa string_view
        }

        std::string_view Update(const domain::Team& entity) override {
            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            nlohmann::json teamDoc = entity;

            pqxx::work tx(*(connection->connection));
            pqxx::result r = tx.exec_params(
                "UPDATE teams SET document = $1 WHERE id = $2::uuid RETURNING id;",
                teamDoc.dump(),
                entity.Id
            );
            tx.commit();

            if (r.empty()) {
                throw std::runtime_error("Team not found");
            }
            return r[0]["id"].c_str();
        }

        void Delete(std::string_view id) override {
            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            pqxx::work tx(*(connection->connection));
            pqxx::result r = tx.exec_params(
                "DELETE FROM teams WHERE id = $1::uuid;",
                id.data()
            );
            tx.commit();

            if (r.affected_rows() == 0) {
                throw std::runtime_error("Team not found");
            }
        }

        // ========================
        // Metodos extra (no-interface)
        // ========================

        // Regresa solo los IDs de equipos del torneo
        std::vector<std::string> ListIdsByTournament(const std::string& tournamentId) {
            std::vector<std::string> out;

            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            pqxx::work tx(*(connection->connection));
            // Ajusta si usas columna normal tournament_id
            pqxx::result r = tx.exec_params(
                "SELECT id FROM teams WHERE document->>'tournamentId' = $1;",
                tournamentId
            );
            tx.commit();

            out.reserve(r.size());
            for (const auto& row : r) {
                out.emplace_back(row[0].as<std::string>());
            }
            return out;
        }

        // Version que devuelve objetos Team ligeros (Id, name) para tu processor actual
        std::vector<domain::Team> ListByTournament(const std::string& tournamentId) {
            std::vector<domain::Team> out;

            auto pooled = connectionProvider->Connection();
            auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

            pqxx::work tx(*(connection->connection));
            // Ajusta si usas columna normal tournament_id
            pqxx::result r = tx.exec_params(
                "SELECT id, document->>'name' AS name "
                "FROM teams "
                "WHERE document->>'tournamentId' = $1;",
                tournamentId
            );
            tx.commit();

            out.reserve(r.size());
            for (const auto& row : r) {
                out.emplace_back(domain::Team{
                    row["id"].c_str(),
                    row["name"].is_null() ? "" : row["name"].c_str()
                });
            }
            return out;
        }
    };

} // namespace persistence

#endif // RESTAPI_TEAMREPOSITORY_HPP

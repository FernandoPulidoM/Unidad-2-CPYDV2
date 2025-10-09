#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <string>

namespace persistence {

    struct Tournament {
        int id;
        std::string name;
    };

    class TournamentRepository {
    public:
        TournamentRepository(const std::string& connectionString);
        void save(const Tournament& tournament);
        std::vector<Tournament> getAll();

        // Agregar en la clase TournamentRepository, después de getAll():
        void deleteById(const std::string& id);

    private:
        std::string connectionString;
    };

} // namespace persistence

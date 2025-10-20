//
// Created by tsuny on 9/1/25.
//

#ifndef TOURNAMENTS_TEAMREPOSITORY_HPP
#define TOURNAMENTS_TEAMREPOSITORY_HPP
#include <string>

#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"


class TeamRepository : public IRepository<domain::Team, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider);
    std::shared_ptr<domain::Team> ReadById(std::string id) override;
    std::string Create (const domain::Team & entity) override;
    // version previa  std::string Update (const domain::Team & entity) override;

    // En la clase TeamRepository:
    // En la clase TeamRepository:
    std::string Update(const domain::Team& entity) override;

    void Delete(std::string id) override;//ya existe
    std::vector<std::shared_ptr<domain::Team>> ReadAll() override;
};

#endif //TOURNAMENTS_TOURNAMENTREPOSITORY_HPP
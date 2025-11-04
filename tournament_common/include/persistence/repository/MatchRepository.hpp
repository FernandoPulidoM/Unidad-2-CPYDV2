#ifndef TOURNAMENTS_MATCHREPOSITORY_HPP
#define TOURNAMENTS_MATCHREPOSITORY_HPP

#include <string>
#include <memory>
#include "persistence/repository/IRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "domain/Match.hpp"

class MatchRepository : public IRepository<domain::Match, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
    
public:
    explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> provider);
    
    std::string Create(const domain::Match& entity) override;
    std::vector<std::shared_ptr<domain::Match>> ReadAll() override;
    std::shared_ptr<domain::Match> ReadById(std::string id) override;
    std::string Update(const domain::Match& entity) override;
    void Delete(std::string id) override;
    
    // Métodos específicos
    std::vector<std::shared_ptr<domain::Match>> FindByTournamentId(const std::string& tournamentId);
};

#endif
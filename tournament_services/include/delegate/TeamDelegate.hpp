#ifndef TOURNAMENTS_TEAMDELEGATE_HPP
#define TOURNAMENTS_TEAMDELEGATE_HPP

#include <memory>
#include <string>
#include "delegate/ITeamDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "cms/QueueMessageProducer.hpp"

class TeamDelegate : public ITeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string>> teamRepository;
    std::shared_ptr<QueueMessageProducer> producer;

public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string>> repository,
                          std::shared_ptr<QueueMessageProducer> producer);
    std::string CreateTeam(std::shared_ptr<domain::Team> team) override;
    std::vector<std::shared_ptr<domain::Team>> ReadAll() override;
    void UpdateTeam(const std::string& id, std::shared_ptr<domain::Team> team) override;
    void DeleteTeam(const std::string& id) override;
};

#endif

#ifndef TOURNAMENTS_ITEAMDELEGATE_HPP
#define TOURNAMENTS_ITEAMDELEGATE_HPP

#include <string>
#include <memory>
#include <vector>
#include "domain/Team.hpp"

class ITeamDelegate {
public:
    virtual ~ITeamDelegate() = default;
    virtual std::string CreateTeam(std::shared_ptr<domain::Team> team) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> ReadAll() = 0;
    virtual void UpdateTeam(const std::string& id, std::shared_ptr<domain::Team> team) = 0;
    virtual void DeleteTeam(const std::string& id) = 0;
};

#endif

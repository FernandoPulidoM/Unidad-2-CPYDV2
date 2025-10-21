#ifndef TOURNAMENTS_GROUPCONTROLLER_HPP
#define TOURNAMENTS_GROUPCONTROLLER_HPP

#include <vector>
#include <string>
#include <memory>
#include <crow.h>

#include "delegate/IGroupDelegate.hpp"

class GroupController {
    std::shared_ptr<IGroupDelegate> groupDelegate;
public:
    explicit GroupController(const std::shared_ptr<IGroupDelegate>& delegate);
    ~GroupController();

    crow::response GetGroups(const std::string& tournamentId);
    crow::response GetGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response CreateGroup(const crow::request& request, const std::string& tournamentId);
    crow::response UpdateGroup(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
    crow::response DeleteGroup(const std::string& tournamentId, const std::string& groupId);
    crow::response UpdateTeams(const crow::request& request, const std::string& tournamentId, const std::string& groupId);
};

#endif
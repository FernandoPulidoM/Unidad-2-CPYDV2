#ifndef TOURNAMENTS_TEAMCONTROLLER_HPP
#define TOURNAMENTS_TEAMCONTROLLER_HPP

#include <crow.h>
#include "delegate/ITeamDelegate.hpp"

class TeamController {
    std::shared_ptr<ITeamDelegate> teamDelegate;

public:
    explicit TeamController(std::shared_ptr<ITeamDelegate> delegate);
    [[nodiscard]] crow::response CreateTeam(const crow::request& request) const;
    [[nodiscard]] crow::response ReadAll() const;
    [[nodiscard]] crow::response UpdateTeam(const crow::request& request, const std::string& id) const;
    [[nodiscard]] crow::response DeleteTeam(const std::string& id) const;
};

#endif

#ifndef DOMAIN_TOURNAMENT_HPP
#define DOMAIN_TOURNAMENT_HPP

#include <string>
#include <vector>

#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {
    enum class TeamType {
        ROUND_ROBIN, NFL
    };

    class TeamFormat {
        int numberOfGroups;
        int maxTeamsPerGroup;
        TeamType type;
    public:
        TeamFormat(int numberOfGroups = 1, int maxTeamsPerGroup = 16, TeamType teamType = TeamType::ROUND_ROBIN) {
            this->numberOfGroups = numberOfGroups;
            this->maxTeamsPerGroup = maxTeamsPerGroup;
            this->type = teamType;
        }

        int NumberOfGroups() const {
            return this->numberOfGroups;
        }
        int & NumberOfGroups() {
            return this->numberOfGroups;
        }

        int MaxTeamsPerGroup() const {
            return this->maxTeamsPerGroup;
        }

        int & MaxTeamsPerGroup() {
            return this->maxTeamsPerGroup;
        }

        TeamType Type() const {
            return this->type;
        }

        TeamType & Type() {
            return this->type;
        }
    };

    class Team
    {
        std::string id;
        std::string name;
        TeamFormat format;
        std::vector<Group> groups;
        std::vector<Match> matches;

    public:
        explicit Team(const std::string &name = "", const TeamFormat& format = TeamFormat()) {
            this->name = name;
            this->format = format;
        }

        [[nodiscard]] std::string Id() const {
            return this->id;
        }

        std::string& Id() {
            return this->id;
        }

        [[nodiscard]] std::string Name() const {
            return this->name;
        }

        std::string& Name() {
            return this->name;
        }

        [[nodiscard]] TeamFormat Format() const {
            return this->format;
        }

        TeamFormat & Format () {
            return this->format;
        }

        [[nodiscard]] std::vector<Group> & Groups() {
            return this->groups;
        }

        [[nodiscard]] std::vector<Match> Matches() const {
            return this->matches;
        }
    };
}
#endif
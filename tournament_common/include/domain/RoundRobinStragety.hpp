//
// RoundRobinStrategy.hpp
//
#pragma once
#include "IMatchStrategy.hpp"
#include <algorithm>
#include <cmath>

class RoundRobinStrategy : public IMatchStrategy {
public:
    std::expected<std::vector<domain::Match>, std::string>
    GenerateMatches(const std::string& tournamentId,
                    const std::vector<std::shared_ptr<domain::Group>>& groups) override {

        std::vector<domain::Match> allMatches;

        // Fase 1: Round-robin dentro de cada grupo
        for (const auto& group : groups) {
            auto groupMatches = generateGroupRoundRobin(tournamentId, group);
            if (!groupMatches.has_value()) {
                return std::unexpected(groupMatches.error());
            }
            allMatches.insert(allMatches.end(),
                            groupMatches->begin(),
                            groupMatches->end());
        }

        return allMatches;
    }

private:
    std::expected<std::vector<domain::Match>, std::string>
    generateGroupRoundRobin(const std::string& tournamentId,
                           const std::shared_ptr<domain::Group>& group) {

        const auto& teams = group->Teams();
        if (teams.size() < 2) {
            return std::unexpected("Group must have at least 2 teams");
        }

        std::vector<domain::Match> matches;

        // Round-robin: cada equipo juega contra todos los demás
        for (size_t i = 0; i < teams.size(); ++i) {
            for (size_t j = i + 1; j < teams.size(); ++j) {
                domain::Match match;
                match.tournamentId = tournamentId;
                match.homeTeamId = teams[i].Id;
                match.awayTeamId = teams[j].Id;
                match.round = "group_stage"; // o "regular"
                match.status = "pending";
                // No asignar id aquí, lo genera la DB

                matches.push_back(match);
            }
        }

        return matches;
    }
};
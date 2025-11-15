//
// Created by tomas on 8/31/25.
//

#ifndef TOURNAMENTS_IMATCHSTRATEGY_HPP
#define TOURNAMENTS_IMATCHSTRATEGY_HPP

#include <vector>
#include <string>
#include <expected>
#include "domain/Match.hpp"
#include "domain/Group.hpp"

class IMatchStrategy {
public:
    virtual ~IMatchStrategy() = default;

    // Genera partidos para un torneo dado grupos
    virtual std::expected<std::vector<domain::Match>, std::string>
    GenerateMatches(const std::string& tournamentId,
                    const std::vector<std::shared_ptr<domain::Group>>& groups) = 0;
};

#endif //TOURNAMENTS_IMATCHSTRATEGY_HPP
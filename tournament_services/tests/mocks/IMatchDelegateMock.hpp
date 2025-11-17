#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <expected>
#include <nlohmann/json.hpp>

#include "../../../tournament_services/include/delegate/IMatchDelegate.hpp"
#include "../../../tournament_common/include/domain/Match.hpp"

class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD(std::vector<domain::Match>,
                List,
                (const std::string&, const std::optional<std::string>&),
                (override));

    MOCK_METHOD(std::optional<domain::Match>,
                Get,
                (const std::string&, const std::string&),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                updateScore,
                (const std::string&, const std::string&, int, int),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                GenerateMatchesForTournament,
                (const std::string&),
                (override));

    MOCK_METHOD((std::expected<std::vector<TeamStanding>, std::string>),
                GetGroupStandings,
                (const std::string&, const std::string&),
                (override));

    MOCK_METHOD((std::expected<nlohmann::json, std::string>),
                GetTournamentStatus,
                (const std::string&),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                GenerateKnockoutPhase,
                (const std::string&),
                (override));

    MOCK_METHOD((std::expected<void, std::string>),
                AdvanceKnockoutPhase,
                (const std::string&),
                (override));
};
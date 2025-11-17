#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <expected>
#include <nlohmann/json.hpp>

#include "persistence/repository/MatchRepository.hpp"
#include "domain/Match.hpp"

namespace persistence {

    class MockMatchRepository : public MatchRepository {
    public:
        MockMatchRepository() : MatchRepository(nullptr) {}

        MOCK_METHOD(std::vector<domain::Match>,
                    ListByTournament,
                    (const std::string&, const std::optional<std::string>&),
                    (override));

        MOCK_METHOD(std::optional<domain::Match>,
                    GetById,
                    (const std::string&, const std::string&),
                    (override));

        MOCK_METHOD((std::expected<void, std::string>),
                    UpdateScore,
                    (const std::string&, const std::string&, int, int),
                    (override));

        MOCK_METHOD(bool,
                    ExistsPairing,
                    (const std::string&, const std::string&, const std::string&),
                    (override));

        MOCK_METHOD((std::expected<std::string, std::string>),
                    Create,
                    (const nlohmann::json&),
                    (override));

        MOCK_METHOD((std::expected<void, std::string>),
                    CreateBulk,
                    (const std::vector<nlohmann::json>&),
                    (override));
    };

} // namespace persistence
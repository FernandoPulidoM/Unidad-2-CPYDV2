#pragma once
#include <gmock/gmock.h>
#include "domain/IMatchStrategy.hpp"

class MockMatchStrategy : public IMatchStrategy {
public:
    MOCK_METHOD((std::expected<std::vector<domain::Match>, std::string>),
                GenerateMatches,
                (const std::string&, const std::vector<std::shared_ptr<domain::Group>>&),
                (override));
};
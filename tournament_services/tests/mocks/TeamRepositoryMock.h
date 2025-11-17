#pragma once
#include <gmock/gmock.h>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include "../../../tournament_common/include/persistence/repository/TeamRepository.hpp"
#include "../../../tournament_common/include/persistence/configuration/IDbConnectionProvider.hpp"
#include "../../../tournament_common/include/domain/Team.hpp"

class TeamRepositoryMock : public persistence::TeamRepository {
public:
    TeamRepositoryMock()
    : TeamRepository(std::shared_ptr<IDbConnectionProvider>{}) {}

    MOCK_METHOD(std::string_view, Create, (const domain::Team&), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string_view), (override));
    MOCK_METHOD(std::string_view, Update, (const domain::Team&), (override));
    MOCK_METHOD(void, Delete, (std::string_view), (override));
};
using MockTeamRepository = TeamRepositoryMock;